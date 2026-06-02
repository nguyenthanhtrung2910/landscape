#ifndef __HW_VIDEO_ENCODER_H__
#define __HW_VIDEO_ENCODER_H__

#include <cstdint>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <cuda.h>

#include <UnigineComponentSystem.h>
#include <UnigineViewport.h>
#include <UnigineWidgets.h>

#include "cudatexture.h"

struct AVBufferRef;
struct AVCodecContext;
struct AVFormatContext;
struct AVFrame;
struct AVPacket;
struct AVStream;

class HWVideoEncoder : public Unigine::ComponentBase
{
    enum State
    {
        STATE_IDLE = 0,
        STATE_RECORDING,
    };

public:
    COMPONENT_DEFINE(HWVideoEncoder, Unigine::ComponentBase);
    PROP_PARAM(Int, fps, 60);
    COMPONENT_INIT(init);
    // Update renders to custom texture so it should be invoked last in the update order
    COMPONENT_UPDATE(update, 10);

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
    bool openEncoder(const std::string &videoPath, int width, int height);
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
    AVBufferRef *m_hwDeviceContextRef{};
    AVBufferRef *m_hwFramesContextRef{};

    const char *const m_dataPath{
        Unigine::Engine::get()->getDataPath()};

    int m_state{STATE_IDLE};
    int m_width{0};
    int m_height{0};
    int64_t m_frameIndex{0};
    bool m_hwFramesOpened{false};
    bool m_encoderOpened{false};
};

#endif // __HW_VIDEO_ENCODER_H__
