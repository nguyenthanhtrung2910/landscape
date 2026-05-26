#ifndef __VIDEO_MAKER_H__
#define __VIDEO_MAKER_H__

#include <string>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <opencv2/opencv.hpp>

#include <UnigineCallback.h>
#include <UnigineComponentSystem.h>
#include <UnigineViewport.h>
#include <UnigineWidgets.h>

class VideoMaker : public Unigine::ComponentBase
{
    enum State
    {
        STATE_IDLE = 0,
        STATE_WARMUP,
        STATE_RECORDING,
    };

public:
    COMPONENT_DEFINE(VideoMaker, Unigine::ComponentBase);
    PROP_PARAM(IVec2, size, Unigine::Math::ivec2(1920, 1080));
    PROP_PARAM(Int, fps, 30);
    PROP_PARAM(Int, warmup_frames, 10);
    COMPONENT_INIT(init, -1);
    COMPONENT_SHUTDOWN(shutdown);
    COMPONENT_UPDATE(update);

    void startRecording();
    void stopRecording();
    void startRecordingInDefaultPath();

private:
    void init();
    void shutdown();
    void update();
    void encoderLoop();
    void renderWarmupFrame(const Unigine::PlayerPtr &player);
    void renderRecordingFrame(const Unigine::PlayerPtr &player);
    bool openWriter(const std::string &videoPath);
    void finalizeRecording();

private:
    Unigine::WidgetLabelPtr m_recordingMark{};
    Unigine::TexturePtr m_texture{};
    Unigine::ViewportPtr m_viewport{};
    Unigine::EventConnections m_econnections;
    const char *const m_dataPath{Unigine::Engine::get()->getDataPath()};
    std::mutex m_writerMutex;
    cv::VideoWriter m_writer;
    std::mutex m_readyFramesMutex;
    Unigine::HashMap<uint64_t, cv::Mat> m_readyFrames{};
    std::condition_variable m_cv;
    std::thread m_encoderThread;
    uint64_t m_nextFrameToEncode{1};
    uint64_t m_lastSubmittedFrame{0};
    bool m_stopRequested{false};
    bool m_workerShutdown{false};

    int m_warmupCount{0};
    int m_state{STATE_IDLE};
};

#endif // __VIDEO_MAKER_H__
