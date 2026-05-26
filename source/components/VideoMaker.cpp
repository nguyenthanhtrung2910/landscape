#include "VideoMaker.h"

#include <UnigineComponentSystem.h>
#include <UnigineGame.h>
#include <UniginePlayers.h>
#include <UnigineUserInterface.h>
#include <UnigineWindowManager.h>

REGISTER_COMPONENT(VideoMaker);

using namespace Unigine;

void VideoMaker::init()
{
    m_texture = Texture::create();
    m_viewport = Viewport::create();
    m_viewport->setSkipFlags(Viewport::SKIP_VISUALIZER);
    m_viewport->setMode(Render::getViewportMode());
    m_encoderThread = std::thread(&VideoMaker::encoderLoop, this);
    GuiPtr gui{Gui::getCurrent()};

    WidgetGroupBoxPtr groupBox{WidgetGroupBox::create("Video Recording")};
    WidgetVBoxPtr vbox{WidgetVBox::create()};
    groupBox->setPosition(20, 50);
    groupBox->addChild(vbox, Gui::ALIGN_CENTER);
    WidgetButtonPtr buttonStartRecording{WidgetButton::create("Start")};
    buttonStartRecording->getEventClicked().connect(
        m_econnections,
        this,
        &VideoMaker::startRecording);
    WidgetButtonPtr buttonStopRecording{WidgetButton::create("Stop")};
    buttonStopRecording->getEventClicked().connect(
        m_econnections,
        this,
        &VideoMaker::stopRecording);
    m_recordingMark = WidgetLabel::create("Recording!");
    m_recordingMark->setFontSize(24);
    m_recordingMark->setHidden(true);
    m_recordingMark->setFontColor({255.0f, 0.0f, 0.0f, 1.0f});
    vbox->addChild(buttonStartRecording);
    vbox->addChild(buttonStopRecording);
    vbox->addChild(m_recordingMark);
    gui->addChild(groupBox, Gui::ALIGN_OVERLAP);
}

void VideoMaker::shutdown()
{
    stopRecording();
    {
        std::lock_guard<std::mutex> lock(m_readyFramesMutex);
        // Let the encoder thread exit once it drains all submitted frames.
        m_workerShutdown = true;
    }
    m_cv.notify_all();
    if (m_encoderThread.joinable())
        m_encoderThread.join();
}

void VideoMaker::startRecording()
{
    if (m_state != STATE_IDLE)
        return;

    {
        std::lock_guard<std::mutex> lock(m_readyFramesMutex);
        if (m_stopRequested)
            return;
    }

    std::string videoPath{WindowManager::dialogSaveFile(m_dataPath)};
    if (videoPath.empty())
        return;
    if (videoPath.size() < 4 ||
        videoPath.substr(videoPath.size() - 4) != ".mp4")
        videoPath += ".mp4";

    if (!openWriter(videoPath))
    {
        WindowManager::dialogError("Error", "Can't open video writer!");
        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_readyFramesMutex);
        // Reset the ordered frame queue before starting a new recording session.
        m_readyFrames.clear();
        m_nextFrameToEncode = 1;
        m_lastSubmittedFrame = 0;
        m_stopRequested = false;
    }
    m_cv.notify_all();

    m_warmupCount = 0;
    m_state = STATE_WARMUP;

    m_recordingMark->setHidden(false);
}

void VideoMaker::stopRecording()
{
    if (m_state == STATE_IDLE)
        return;

    m_state = STATE_IDLE;

    {
        std::lock_guard<std::mutex> lock(m_readyFramesMutex);
        // Stop accepting new frames but keep draining already submitted transfers.
        m_stopRequested = true;
    }
    m_cv.notify_all();

    m_recordingMark->setHidden(true);
}

void VideoMaker::startRecordingInDefaultPath()
{
    if (m_state != STATE_IDLE)
        return;

    {
        std::lock_guard<std::mutex> lock(m_readyFramesMutex);
        if (m_stopRequested)
            return;
    }

    if (!openWriter(std::string(m_dataPath) + "/video.mp4"))
    {
        WindowManager::dialogError("Error", "Can't open video writer!");
        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_readyFramesMutex);
        // Reset the ordered frame queue before starting a new recording session.
        m_readyFrames.clear();
        m_nextFrameToEncode = 1;
        m_lastSubmittedFrame = 0;
        m_stopRequested = false;
    }
    m_cv.notify_all();

    m_warmupCount = 0;
    m_state = STATE_WARMUP;

    m_recordingMark->setHidden(false);
}

bool VideoMaker::openWriter(const std::string &videoPath)
{
    std::lock_guard<std::mutex> lock(m_writerMutex);
    if (m_writer.isOpened())
        m_writer.release();

    return m_writer.open(
        videoPath,
        cv::VideoWriter::fourcc('m', 'p', '4', 'v'),
        fps,
        cv::Size(size.get().x, size.get().y));
}

void VideoMaker::finalizeRecording()
{
    std::lock_guard<std::mutex> lock(m_writerMutex);
    if (m_writer.isOpened())
        m_writer.release();
}

void VideoMaker::renderWarmupFrame(const PlayerPtr &player)
{
    m_viewport->setMode(Render::getViewportMode());

    if (m_warmupCount == 0)
    {
        // First frame we render with velocity buffer turned off to avoid temporal effects artifacts
        m_viewport->appendSkipFlags(
            Viewport::SKIP_VELOCITY_BUFFER);
        m_viewport->renderTexture2D(
            player->getCamera(),
            m_texture,
            size.get().x,
            size.get().y);
        m_viewport->removeSkipFlags(
            Viewport::SKIP_VELOCITY_BUFFER);
        return;
    }

    // During warmup we temporarily disable exposure adaptation so the captured
    // image stabilizes before we start recording.
    float exposure_adaptation = Render::getExposureAdaptation();
    Render::setExposureAdaptation(0.0f);
    m_viewport->renderTexture2D(
        player->getCamera(),
        m_texture,
        size.get().x,
        size.get().y);
    Render::setExposureAdaptation(exposure_adaptation);
}

void VideoMaker::renderRecordingFrame(const PlayerPtr &player)
{
    m_viewport->renderTexture2D(
        player->getCamera(),
        m_texture,
        size.get().x,
        size.get().y);
}

void VideoMaker::encoderLoop()
{
    for (;;)
    {
        Unigine::Vector<cv::Mat> frames_to_encode;
        bool should_finalize = false;
        bool should_exit = false;

        {
            std::unique_lock<std::mutex> lock(m_readyFramesMutex);
            // Wake the encoder thread in two cases:
            // 1. The next frame in sequence is ready, so encoding can continue in order.
            // 2. Recording/shutdown requested and every submitted frame id has already
            //    been encoded, so no additional frame can still arrive or remain queued.
            m_cv.wait(lock, [this]
                      { return m_readyFrames.contains(m_nextFrameToEncode) ||
                               ((m_stopRequested || m_workerShutdown) &&
                                m_nextFrameToEncode > m_lastSubmittedFrame); });

            // Drain every contiguous ready frame so one notify can encode a whole batch.
            while (m_readyFrames.contains(m_nextFrameToEncode))
            {
                frames_to_encode.append(
                    m_readyFrames[m_nextFrameToEncode]);
                m_readyFrames.remove(m_nextFrameToEncode);
                ++m_nextFrameToEncode;
            }

            should_finalize =
                m_stopRequested &&
                m_nextFrameToEncode > m_lastSubmittedFrame;
            should_exit =
                m_workerShutdown &&
                m_nextFrameToEncode > m_lastSubmittedFrame;

            if (should_finalize)
                m_stopRequested = false;
        }

        for (const cv::Mat &frame : frames_to_encode)
        {
            std::lock_guard<std::mutex> lock(m_writerMutex);
            if (m_writer.isOpened())
                m_writer << frame;
        }

        if (should_finalize)
            finalizeRecording();

        if (should_exit)
            break;
    }

    finalizeRecording();
}

void VideoMaker::update()
{
    // If we always take view from main player, get frame from display framebuffer is enough.
    // But if we want to support recording from other players, we need to render to external offscreen framebuffer.
    // That is advantage of current approach: take view from custom framebuffer rather than display framebuffer,
    // so we can add more configurations rather than always use main ones.
    PlayerPtr player{Game::getPlayer()};
    if (!player)
    {
        m_state = STATE_IDLE;
        return;
    }
    switch (m_state)
    {
    case STATE_WARMUP:
        renderWarmupFrame(player);
        ++m_warmupCount;
        if (m_warmupCount == warmup_frames.get())
            m_state = STATE_RECORDING;
        break;

    case STATE_RECORDING:
    {
        renderRecordingFrame(player);

        uint64_t frameCount{};
        const bool should_flip = !Render::isFlipped();
        {
            std::lock_guard<std::mutex> lock(m_readyFramesMutex);
            // Frame ids are assigned on submission so the encoder can restore order later.
            frameCount = ++m_lastSubmittedFrame;
        }
        Render::asyncTransferTextureToImage(
            nullptr,
            MakeCallback(
                [this, frameCount, should_flip](ImagePtr image)
                {
                    // Copy into an owned cv::Mat here so the encoder thread never touches
                    // UNIGINE Image objects or engine-managed pixel memory.
                    if (image->getFormat() != Image::FORMAT_RGB8)
                        image->convertToFormat(Image::FORMAT_RGB8);
                    if (should_flip)
                        image->flipY();

                    cv::Mat rgb_frame(
                        image->getHeight(),
                        image->getWidth(),
                        CV_8UC3,
                        image->getPixels());
                    cv::Mat bgr_frame;
                    cv::cvtColor(rgb_frame, bgr_frame, cv::COLOR_RGB2BGR);
                    {
                        std::lock_guard lock(m_readyFramesMutex);
                        // The hash map may fill out of order; frameCount keeps encode order stable.
                        m_readyFrames[frameCount] = std::move(bgr_frame);
                    }
                    m_cv.notify_all();
                }),
            m_texture);
        break;
    }

    case STATE_IDLE:
    default:
        break;
    }
}
