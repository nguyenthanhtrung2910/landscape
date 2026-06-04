#ifndef __RTP_STREAM_H__
#define __RTP_STREAM_H__

#include <cuda.h>

#include <UnigineComponentSystem.h>
#include <UnigineViewport.h>
#include <UnigineWidgets.h>

#include "cudatexture.h"

struct AVBufferRef;
struct AVBSFContext;
struct AVCodecContext;
struct AVFormatContext;
struct AVFrame;
struct AVPacket;
struct AVStream;

class RTPStream : public Unigine::ComponentBase
{
    enum State
    {
        STATE_IDLE = 0,
        STATE_STREAMING,
    };

public:
    COMPONENT_DEFINE(RTPStream, Unigine::ComponentBase);
    PROP_PARAM(Int, fps, 60);
    PROP_PARAM(String, rtp_url, "rtp://127.0.0.1:5004?rtcpport=5005&localrtpport=5104&localrtcpport=5105&pkt_size=1200");
    COMPONENT_INIT(init);
    // Update renders to custom texture so it should be invoked last in the update order
    COMPONENT_UPDATE(update, 10);
    COMPONENT_SHUTDOWN(shutdown);

public:
    void startRecording();
    void stopRecording();
    void shutdown();

private:
    void init();
    void update();

private:
    void drawUI();
    bool openHardwareFrames(int width, int height);
    void closeHardwareFrames();
    bool openEncoder(const char *url, int width, int height);
    bool openBitstreamFilter();
    void closeEncoder();
    bool sendFrame(AVFrame *frame);
    bool writePacket(AVPacket *packet);
    void encodeFrame(int width, int height);

private:
    Unigine::TexturePtr m_texture{};
    Unigine::ViewportPtr m_viewport{};
    Unigine::WidgetLabelPtr m_recordingMark{};
    Unigine::WidgetSpritePtr m_mainWidget{};
    Unigine::EventConnections m_econnections{};
    CUDATexture m_cudaTexture{};

    AVFormatContext *m_formatContext{};
    AVCodecContext *m_codecContext{};
    AVStream *m_videoStream{};
    AVBSFContext *m_bitstreamFilterContext{};
    AVBufferRef *m_hwDeviceContextRef{};
    AVBufferRef *m_hwFramesContextRef{};

    int m_state{STATE_IDLE};
    int m_width{0};
    int m_height{0};
    int64_t m_frameIndex{0};
    bool m_hwFramesOpened{false};
    bool m_encoderOpened{false};
};

#endif // __RTP_STREAM_H__
