#include "hwvideoencoder.h"

#include <string>
extern "C"
{
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

REGISTER_COMPONENT(HWVideoEncoder);

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
        Log::error("HWVideoEncoder: %s: %s\n", what, avError(error).c_str());
    }
} // namespace

void HWVideoEncoder::init()
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
        Log::error("HWVideoEncoder: failed to initialize CUDA hardware frame pool\n");
}

void HWVideoEncoder::drawUI()
{
    GuiPtr gui = Gui::getCurrent();

    // main widget
    Render::setEnabled(false);
    m_mainWidget = WidgetSprite::create(gui);
    // displayed texture with alpha channel, disable widgets blending
    m_mainWidget->setBlendFunc(Gui::BLEND_NONE, Gui::BLEND_NONE);
    gui->addChild(m_mainWidget);

    // recording buttons
    WidgetGroupBoxPtr groupBox{WidgetGroupBox::create("Video Recording")};
    WidgetVBoxPtr vbox{WidgetVBox::create()};
    groupBox->setPosition(20, 50);
    groupBox->addChild(vbox, Gui::ALIGN_CENTER);
    WidgetButtonPtr buttonStartRecording{WidgetButton::create("Start")};
    buttonStartRecording->getEventClicked().connect(
        m_econnections,
        this,
        &HWVideoEncoder::startRecording);
    WidgetButtonPtr buttonStopRecording{WidgetButton::create("Stop")};
    buttonStopRecording->getEventClicked().connect(
        m_econnections,
        this,
        &HWVideoEncoder::stopRecording);
    m_recordingMark = WidgetLabel::create("Recording!");
    m_recordingMark->setFontSize(24);
    m_recordingMark->setHidden(true);
    m_recordingMark->setFontColor({255.0f, 0.0f, 0.0f, 1.0f});
    vbox->addChild(buttonStartRecording);
    vbox->addChild(buttonStopRecording);
    vbox->addChild(m_recordingMark);
    gui->addChild(groupBox, Gui::ALIGN_OVERLAP);
}

void HWVideoEncoder::shutdown()
{
    closeEncoder();
    closeHardwareFrames();
    m_cudaTexture.clear();
}

void HWVideoEncoder::update()
{
    m_viewport->renderTexture2D(Game::getPlayer()->getCamera(), m_texture);
    m_mainWidget->setRender(m_texture, !Render::isFlipped());
    if (m_state == STATE_RECORDING)
        encodeFrame(m_width, m_height);
}

void HWVideoEncoder::startRecording()
{
    if (m_state != STATE_IDLE)
        return;
    std::string videoPath{WindowManager::dialogSaveFile(m_dataPath)};
    if (videoPath.empty())
        return;
    if (videoPath.size() < 4 || videoPath.substr(videoPath.size() - 4) != ".mp4")
        videoPath += ".mp4";
    if (!openEncoder(videoPath, m_width, m_height))
    {
        WindowManager::dialogError("Error", "Can't open hardware video encoder!");
        return;
    }
    m_frameIndex = 0;
    m_state = STATE_RECORDING;
    m_recordingMark->setHidden(false);
}

void HWVideoEncoder::stopRecording()
{
    if (m_state == STATE_IDLE)
        return;
    m_state = STATE_IDLE;
    closeEncoder();
    m_recordingMark->setHidden(true);
}

bool HWVideoEncoder::openHardwareFrames(int width, int height)
{
    if (m_hwFramesOpened)
        return true;

    if ((width & 1) != 0 || (height & 1) != 0)
    {
        Log::error("HWVideoEncoder: NV12 requires even frame size, got %dx%d\n", width, height);
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
        Log::error("HWVideoEncoder: av_hwframe_ctx_alloc failed\n");
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

void HWVideoEncoder::closeHardwareFrames()
{
    av_buffer_unref(&m_hwFramesContextRef);
    av_buffer_unref(&m_hwDeviceContextRef);
    m_hwFramesOpened = false;
}

bool HWVideoEncoder::openEncoder(const std::string &videoPath, int width, int height)
{
    if (m_encoderOpened)
        return true;

    if (!m_hwFramesOpened)
    {
        Log::error("HWVideoEncoder: CUDA hardware frame pool is not initialized\n");
        return false;
    }
    // Allocate format context
    int result = avformat_alloc_output_context2(&m_formatContext, nullptr, nullptr, videoPath.c_str());
    if (result < 0 || !m_formatContext)
    {
        logAVError("avformat_alloc_output_context2", result);
        closeEncoder();
        return false;
    }

    const AVCodec *codec = avcodec_find_encoder_by_name("h264_nvenc");
    if (!codec)
    {
        Log::error("HWVideoEncoder: h264_nvenc encoder was not found by FFmpeg\n");
        closeEncoder();
        return false;
    }

    m_videoStream = avformat_new_stream(m_formatContext, nullptr);
    if (!m_videoStream)
    {
        Log::error("HWVideoEncoder: avformat_new_stream failed\n");
        closeEncoder();
        return false;
    }
    // Allocate codec context
    m_codecContext = avcodec_alloc_context3(codec);
    if (!m_codecContext)
    {
        Log::error("HWVideoEncoder: avcodec_alloc_context3 failed\n");
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
    m_codecContext->bit_rate = 8'000'000;
    m_videoStream->time_base = timeBase;

    if ((m_formatContext->oformat->flags & AVFMT_GLOBALHEADER) != 0)
        m_codecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    m_codecContext->hw_frames_ctx = av_buffer_ref(m_hwFramesContextRef);
    if (m_codecContext->hw_frames_ctx == nullptr)
    {
        Log::error("HWVideoEncoder: av_buffer_ref(hw_frames_ctx) failed\n");
        closeEncoder();
        return false;
    }

    AVDictionary *codecOptions{};
    av_dict_set(&codecOptions, "preset", "p4", 0);
    av_dict_set(&codecOptions, "tune", "ll", 0);
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
    if ((m_formatContext->oformat->flags & AVFMT_NOFILE) == 0)
    {
        result = avio_open(&m_formatContext->pb, videoPath.c_str(), AVIO_FLAG_WRITE);
        if (result < 0)
        {
            logAVError("avio_open", result);
            closeEncoder();
            return false;
        }
    }
    result = avformat_write_header(m_formatContext, nullptr);
    if (result < 0)
    {
        logAVError("avformat_write_header", result);
        closeEncoder();
        return false;
    }
    m_encoderOpened = true;
    return true;
}

void HWVideoEncoder::closeEncoder()
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

    avcodec_free_context(&m_codecContext);
    avformat_free_context(m_formatContext);
    m_formatContext = nullptr;
    m_videoStream = nullptr;
    m_encoderOpened = false;
}

bool HWVideoEncoder::sendFrame(AVFrame *frame)
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
        Log::error("HWVideoEncoder: av_packet_alloc failed\n");
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

bool HWVideoEncoder::writePacket(AVPacket *packet)
{
    packet->stream_index = m_videoStream->index;
    av_packet_rescale_ts(packet, m_codecContext->time_base, m_videoStream->time_base);
    const int result = av_interleaved_write_frame(m_formatContext, packet);
    if (result < 0)
    {
        logAVError("av_interleaved_write_frame", result);
        return false;
    }
    return true;
}

void HWVideoEncoder::encodeFrame(int width, int height)
{
    AVFrame *frame = av_frame_alloc();
    if (!frame)
    {
        Log::error("HWVideoEncoder: av_frame_alloc failed\n");
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
            "HWVideoEncoder: RGBA to NV12 conversion failed: %s\n",
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