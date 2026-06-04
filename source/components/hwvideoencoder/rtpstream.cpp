#include "rtpstream.h"

#include <string>
extern "C"
{
#include <libavcodec/bsf.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/dict.h>
#include <libavutil/error.h>
#include <libavutil/frame.h>
#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_cuda.h>
#include <libavutil/opt.h>
}

#include <UnigineGame.h>
#include <UnigineRender.h>
#include <UnigineWindowManager.h>

#include "cudautils.h"
#include "rgba_to_nv12.h"

REGISTER_COMPONENT(RTPStream);

using namespace Unigine;

namespace
{
    std::string avError(int error)
    {
        char buffer[AV_ERROR_MAX_STRING_SIZE]{};
        av_strerror(error, buffer, sizeof(buffer));
        return buffer;
    }
    void logAVError(const char *what, int error)
    {
        Log::error("RTPStream: %s: %s\n", what, avError(error).c_str());
    }
} // namespace

void RTPStream::init()
{
    Math::ivec2 window_size{WindowManager::getMainWindow()->getSize()};
    m_width = window_size.x & ~1;
    m_height = window_size.y & ~1;
    m_texture = Texture::create();
    m_texture->create2D(m_width, m_height, Texture::FORMAT_RGBA8,
                        Texture::FORMAT_USAGE_RENDER | Texture::FORMAT_USAGE_SHARED);
    m_cudaTexture.exportToCuda(m_texture);
    m_viewport = Render::getViewportMain();
    drawUI();

    if (!openHardwareFrames(m_width, m_height))
        Log::error("RTPStream: failed to initialize CUDA hardware frame pool\n");
}

void RTPStream::drawUI()
{
    GuiPtr gui = Gui::getCurrent();

    // main widget
    Render::setEnabled(false);
    m_mainWidget = WidgetSprite::create(gui);
    // displayed texture with alpha channel, disable widgets blending
    m_mainWidget->setBlendFunc(Gui::BLEND_NONE, Gui::BLEND_NONE);
    gui->addChild(m_mainWidget);

    // streaming buttons
    WidgetGroupBoxPtr groupBox{WidgetGroupBox::create("RTP Stream")};
    WidgetVBoxPtr vbox{WidgetVBox::create()};
    groupBox->setPosition(20, 50);
    groupBox->addChild(vbox, Gui::ALIGN_CENTER);
    WidgetButtonPtr buttonStartRecording{WidgetButton::create("Start")};
    buttonStartRecording->getEventClicked().connect(
        m_econnections,
        this,
        &RTPStream::startRecording);
    WidgetButtonPtr buttonStopRecording{WidgetButton::create("Stop")};
    buttonStopRecording->getEventClicked().connect(
        m_econnections,
        this,
        &RTPStream::stopRecording);
    m_recordingMark = WidgetLabel::create("Streaming!");
    m_recordingMark->setFontSize(24);
    m_recordingMark->setHidden(true);
    m_recordingMark->setFontColor({255.0f, 0.0f, 0.0f, 1.0f});
    vbox->addChild(buttonStartRecording);
    vbox->addChild(buttonStopRecording);
    vbox->addChild(m_recordingMark);
    gui->addChild(groupBox, Gui::ALIGN_OVERLAP);
}

void RTPStream::shutdown()
{
    closeEncoder();
    closeHardwareFrames();
    m_cudaTexture.clear();
}

void RTPStream::update()
{
    m_viewport->renderTexture2D(Game::getPlayer()->getCamera(), m_texture);
    m_mainWidget->setRender(m_texture, !Render::isFlipped());
    if (m_state == STATE_STREAMING)
        encodeFrame(m_width, m_height);
}

void RTPStream::startRecording()
{
    if (m_state != STATE_IDLE)
        return;
    if (!openEncoder(rtp_url.get(), m_width, m_height))
    {
        WindowManager::dialogError("Error", "Can't open RTP hardware video encoder!");
        return;
    }
    m_frameIndex = 0;
    m_state = STATE_STREAMING;
    m_recordingMark->setHidden(false);
}

void RTPStream::stopRecording()
{
    if (m_state == STATE_IDLE)
        return;
    m_state = STATE_IDLE;
    closeEncoder();
    m_recordingMark->setHidden(true);
}

bool RTPStream::openHardwareFrames(int width, int height)
{
    if (m_hwFramesOpened)
        return true;

    if ((width & 1) != 0 || (height & 1) != 0)
    {
        Log::error("RTPStream: NV12 requires even frame size, got %dx%d\n", width, height);
        return false;
    }
    // Allocate device context
    AVDictionary *deviceOptions{};
    av_dict_set(&deviceOptions, "current_ctx", "1", 0);
    int result = av_hwdevice_ctx_create(
        &m_hwDeviceContextRef,
        AV_HWDEVICE_TYPE_CUDA,
        nullptr,
        deviceOptions,
        0);
    av_dict_free(&deviceOptions);
    if (result < 0)
    {
        logAVError("av_hwdevice_ctx_create(cuda)", result);
        closeHardwareFrames();
        return false;
    }
    // Allocate frames context
    m_hwFramesContextRef = av_hwframe_ctx_alloc(m_hwDeviceContextRef);
    if (!m_hwFramesContextRef)
    {
        Log::error("RTPStream: av_hwframe_ctx_alloc failed\n");
        closeHardwareFrames();
        return false;
    }

    AVHWFramesContext *framesContext =
        reinterpret_cast<AVHWFramesContext *>(m_hwFramesContextRef->data);
    framesContext->format = AV_PIX_FMT_CUDA;
    framesContext->sw_format = AV_PIX_FMT_NV12;
    framesContext->width = width;
    framesContext->height = height;
    framesContext->initial_pool_size = 8;
    result = av_hwframe_ctx_init(m_hwFramesContextRef);
    if (result < 0)
    {
        logAVError("av_hwframe_ctx_init", result);
        closeHardwareFrames();
        return false;
    }

    m_hwFramesOpened = true;
    return true;
}

void RTPStream::closeHardwareFrames()
{
    av_buffer_unref(&m_hwFramesContextRef);
    av_buffer_unref(&m_hwDeviceContextRef);
    m_hwFramesOpened = false;
}

bool RTPStream::openEncoder(const char *url, int width, int height)
{
    if (m_encoderOpened)
        return true;

    if (!m_hwFramesOpened)
    {
        Log::error("RTPStream: CUDA hardware frame pool is not initialized\n");
        return false;
    }
    // Allocate format context
    int result = avformat_alloc_output_context2(&m_formatContext, nullptr, "rtp", url);
    if (result < 0 || !m_formatContext)
    {
        logAVError("avformat_alloc_output_context2", result);
        closeEncoder();
        return false;
    }

    const AVCodec *codec = avcodec_find_encoder_by_name("h264_nvenc");
    if (!codec)
    {
        Log::error("RTPStream: h264_nvenc encoder was not found by FFmpeg\n");
        closeEncoder();
        return false;
    }

    m_videoStream = avformat_new_stream(m_formatContext, nullptr);
    if (!m_videoStream)
    {
        Log::error("RTPStream: avformat_new_stream failed\n");
        closeEncoder();
        return false;
    }
    // Allocate codec context
    m_codecContext = avcodec_alloc_context3(codec);
    if (!m_codecContext)
    {
        Log::error("RTPStream: avcodec_alloc_context3 failed\n");
        closeEncoder();
        return false;
    }

    const AVRational timeBase{1, fps.get()};
    m_codecContext->width = width;
    m_codecContext->height = height;
    m_codecContext->time_base = timeBase;
    m_codecContext->framerate = AVRational{fps.get(), 1};
    m_codecContext->pix_fmt = AV_PIX_FMT_CUDA;
    m_codecContext->sw_pix_fmt = AV_PIX_FMT_NV12;
    m_codecContext->color_range = AVCOL_RANGE_MPEG;
    m_codecContext->colorspace = AVCOL_SPC_BT709;
    m_codecContext->color_primaries = AVCOL_PRI_BT709;
    m_codecContext->color_trc = AVCOL_TRC_BT709;
    m_codecContext->gop_size = fps.get();
    m_codecContext->max_b_frames = 0;
    m_codecContext->bit_rate = 4'000'000;
    m_codecContext->rc_max_rate = 4'000'000;
    m_codecContext->rc_buffer_size = 1'000'000;
    m_videoStream->time_base = timeBase;

    if ((m_formatContext->oformat->flags & AVFMT_GLOBALHEADER) != 0)
        m_codecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    m_codecContext->hw_frames_ctx = av_buffer_ref(m_hwFramesContextRef);
    if (m_codecContext->hw_frames_ctx == nullptr)
    {
        Log::error("RTPStream: av_buffer_ref(hw_frames_ctx) failed\n");
        closeEncoder();
        return false;
    }

    AVDictionary *codecOptions{};
    av_dict_set(&codecOptions, "preset", "p4", 0);
    av_dict_set(&codecOptions, "tune", "ll", 0);
    av_dict_set(&codecOptions, "zerolatency", "1", 0);
    av_dict_set(&codecOptions, "profile", "baseline", 0);
    av_dict_set(&codecOptions, "level", "4.2", 0);
    av_dict_set(&codecOptions, "forced-idr", "1", 0);
    av_dict_set(&codecOptions, "rc", "cbr", 0);
    result = avcodec_open2(m_codecContext, codec, &codecOptions);
    av_dict_free(&codecOptions);
    if (result < 0)
    {
        logAVError("avcodec_open2(h264_nvenc)", result);
        closeEncoder();
        return false;
    }
    result = avcodec_parameters_from_context(m_videoStream->codecpar, m_codecContext);
    if (result < 0)
    {
        logAVError("avcodec_parameters_from_context", result);
        closeEncoder();
        return false;
    }
    if (!openBitstreamFilter())
    {
        closeEncoder();
        return false;
    }
    if ((m_formatContext->oformat->flags & AVFMT_NOFILE) == 0)
    {
        result = avio_open(&m_formatContext->pb, url, AVIO_FLAG_WRITE);
        if (result < 0)
        {
            logAVError("avio_open", result);
            closeEncoder();
            return false;
        }
    }
    AVDictionary *muxerOptions{};
    av_dict_set(&muxerOptions, "payload_type", "102", 0);
    av_dict_set(&muxerOptions, "ssrc", "22222222", 0);
    result = avformat_write_header(m_formatContext, &muxerOptions);
    av_dict_free(&muxerOptions);
    if (result < 0)
    {
        logAVError("avformat_write_header", result);
        closeEncoder();
        return false;
    }
    m_encoderOpened = true;
    return true;
}

bool RTPStream::openBitstreamFilter()
{
    int result = av_bsf_list_parse_str("dump_extra=freq=keyframe", &m_bitstreamFilterContext);
    if (result < 0 || !m_bitstreamFilterContext)
    {
        logAVError("av_bsf_list_parse_str(dump_extra)", result);
        return false;
    }

    result = avcodec_parameters_copy(m_bitstreamFilterContext->par_in, m_videoStream->codecpar);
    if (result < 0)
    {
        logAVError("avcodec_parameters_copy(bsf par_in)", result);
        return false;
    }
    m_bitstreamFilterContext->time_base_in = m_codecContext->time_base;

    result = av_bsf_init(m_bitstreamFilterContext);
    if (result < 0)
    {
        logAVError("av_bsf_init(dump_extra)", result);
        return false;
    }

    result = avcodec_parameters_copy(m_videoStream->codecpar, m_bitstreamFilterContext->par_out);
    if (result < 0)
    {
        logAVError("avcodec_parameters_copy(bsf par_out)", result);
        return false;
    }
    return true;
}

void RTPStream::closeEncoder()
{
    if (m_encoderOpened)
    {
        sendFrame(nullptr);
        int result = av_write_trailer(m_formatContext);
        if (result < 0)
            logAVError("av_write_trailer", result);
    }

    if (m_formatContext != nullptr && (m_formatContext->oformat->flags & AVFMT_NOFILE) == 0)
        avio_closep(&m_formatContext->pb);

    av_bsf_free(&m_bitstreamFilterContext);
    avcodec_free_context(&m_codecContext);
    avformat_free_context(m_formatContext);
    m_formatContext = nullptr;
    m_videoStream = nullptr;
    m_encoderOpened = false;
}

bool RTPStream::sendFrame(AVFrame *frame)
{
    int result = avcodec_send_frame(m_codecContext, frame);
    if (result < 0)
    {
        logAVError("avcodec_send_frame", result);
        return false;
    }
    AVPacket *packet = av_packet_alloc();
    if (packet == nullptr)
    {
        Log::error("RTPStream: av_packet_alloc failed\n");
        return false;
    }
    bool ok{true};
    for (;;)
    {
        result = avcodec_receive_packet(m_codecContext, packet);
        if (result == AVERROR(EAGAIN) || result == AVERROR_EOF)
            break;
        if (result < 0)
        {
            logAVError("avcodec_receive_packet", result);
            ok = false;
            break;
        }
        if (!writePacket(packet))
            ok = false;
        av_packet_unref(packet);
    }
    av_packet_free(&packet);
    return ok;
}

bool RTPStream::writePacket(AVPacket *packet)
{
    int result = av_bsf_send_packet(m_bitstreamFilterContext, packet);
    if (result < 0)
    {
        logAVError("av_bsf_send_packet", result);
        return false;
    }
    AVPacket *filteredPacket = av_packet_alloc();
    if (filteredPacket == nullptr)
    {
        Log::error("RTPStream: av_packet_alloc failed\n");
        return false;
    }

    bool ok{true};
    for (;;)
    {
        result = av_bsf_receive_packet(m_bitstreamFilterContext, filteredPacket);
        if (result == AVERROR(EAGAIN) || result == AVERROR_EOF)
            break;
        if (result < 0)
        {
            logAVError("av_bsf_receive_packet", result);
            ok = false;
            break;
        }

        filteredPacket->stream_index = m_videoStream->index;
        av_packet_rescale_ts(
            filteredPacket,
            m_bitstreamFilterContext->time_base_out,
            m_videoStream->time_base);
        result = av_interleaved_write_frame(m_formatContext, filteredPacket);
        if (result < 0)
        {
            logAVError("av_interleaved_write_frame", result);
            ok = false;
            break;
        }
        av_packet_unref(filteredPacket);
    }
    av_packet_free(&filteredPacket);
    return ok;
}

void RTPStream::encodeFrame(int width, int height)
{
    AVFrame *frame = av_frame_alloc();
    if (!frame)
    {
        Log::error("RTPStream: av_frame_alloc failed\n");
        stopRecording();
        return;
    }
    int result = av_hwframe_get_buffer(m_hwFramesContextRef, frame, 0);
    if (result < 0)
    {
        logAVError("av_hwframe_get_buffer", result);
        av_frame_free(&frame);
        stopRecording();
        return;
    }
    frame->pts = m_frameIndex++;
    if (fps.get() > 0 && (frame->pts % fps.get()) == 0)
        frame->pict_type = AV_PICTURE_TYPE_I;
    // This operation has to be done in main thread because only in there we can get
    // the frame in right time when world is updated
    cudaError_t cudaResult = rgba8SurfaceToNV12(
        m_cudaTexture.getSurface(),
        frame->data[0],
        static_cast<std::size_t>(frame->linesize[0]),
        width,
        height,
        nullptr);
    if (cudaResult == cudaSuccess)
        cudaResult = cudaDeviceSynchronize();
    if (cudaResult != cudaSuccess)
    {
        Log::error(
            "RTPStream: RGBA to NV12 conversion failed: %s\n",
            cudaGetErrorString(cudaResult));
        av_frame_free(&frame);
        stopRecording();
        return;
    }
    // TODO: For now, the frame has display resolution so this operation can be fast enough.
    // If the resolution will be higher, consider move this logic to separate thread and
    // use frame queue to avoid blocking main thread on encoding.
    if (!sendFrame(frame))
        stopRecording();
    av_frame_free(&frame);
}
