#include "imagegenerator.h"

#include <UnigineGame.h>
#include <UnigineInput.h>
#include <UnigineRender.h>
#include <UnigineUserInterface.h>
#include <UnigineWindowManager.h>

using namespace Unigine;
using namespace Math;

REGISTER_COMPONENT(ImageGenerator)

void ImageGenerator::init()
{
    m_viewport = Viewport::create();
    m_viewport->setMode(Render::getViewportMode());
    m_viewport->getEventEndAuxiliaryDecals().connect(this, &ImageGenerator::captureAuxiliary);
    m_colorTexture = Texture::create();
    m_targetPlayer = checked_ptr_cast<Player>(target_player.get());
    m_framesUntilCapture = capture_frequency.get();

    // Create UI button for manual capture with warmup
    GuiPtr gui{Gui::getCurrent()};
    WidgetGroupBoxPtr groupBox{WidgetGroupBox::create("Image Capture")};
    WidgetVBoxPtr vbox{WidgetVBox::create()};
    groupBox->setPosition(20, 50);
    groupBox->addChild(vbox, Gui::ALIGN_CENTER);
    WidgetButtonPtr buttonCapture{WidgetButton::create("Capture")};
    buttonCapture->getEventClicked().connect(this, &ImageGenerator::startCapture);
    WidgetButtonPtr buttonStop{WidgetButton::create("Stop")};
    buttonStop->getEventClicked().connect(this, &ImageGenerator::stopCapture);
    vbox->addChild(buttonCapture);
    vbox->addChild(buttonStop);
    gui->addChild(groupBox, Gui::ALIGN_OVERLAP);
}

void ImageGenerator::shutdown()
{
}

void ImageGenerator::update()
{
    // Manual capture mode with warmup
    if (m_state == STATE_WARMUP)
    {
        const ivec2 target_size = size.get();
        // First warmup frame: disable velocity buffer to avoid temporal artifacts
        if (m_warmupCount == 0)
        {
            m_viewport->appendSkipFlags(Viewport::SKIP_VELOCITY_BUFFER);
            m_viewport->renderTexture2D(
                m_targetPlayer->getCamera(),
                m_colorTexture,
                target_size.x,
                target_size.y);
            m_viewport->removeSkipFlags(Viewport::SKIP_VELOCITY_BUFFER);
        }
        else
        {
            // Subsequent warmup frames: disable exposure adaptation for stability
            float exposure_adaptation = Render::getExposureAdaptation();
            Render::setExposureAdaptation(0.0f);
            m_viewport->renderTexture2D(
                m_targetPlayer->getCamera(),
                m_colorTexture,
                target_size.x,
                target_size.y);
            Render::setExposureAdaptation(exposure_adaptation);
        }

        ++m_warmupCount;
        if (m_warmupCount == warmup_frames.get())
            m_state = STATE_CAPTURING;
        return;
    }
    if (m_state == STATE_CAPTURING)
    {
        const ivec2 target_size = size.get();
        if (m_framesUntilCapture <= 1)
        {
            m_activeCaptureIndex = ++m_captureIndex;
            m_viewport->renderTexture2D(
                m_targetPlayer->getCamera(),
                m_colorTexture,
                target_size.x,
                target_size.y);
            saveTextureAsync(
                m_colorTexture,
                "unigine_dataset_test_1/image_%d.jpg",
                m_activeCaptureIndex);
            m_framesUntilCapture = capture_frequency.get();
        }
        else
        {
            m_viewport->renderTexture2D(
                m_targetPlayer->getCamera(),
                m_colorTexture,
                target_size.x,
                target_size.y);
            --m_framesUntilCapture;
        }
    }
}

void ImageGenerator::captureAuxiliary()
{
    if (m_framesUntilCapture <= 1)
    {
        TexturePtr auxiliaryTexture = Renderer::getTextureAuxiliary();
        if (!auxiliaryTexture)
        {
            Log::warning("ImageGenerator: auxiliary texture is unavailable.\n");
            return;
        }
        saveTextureAsync(
            auxiliaryTexture,
            "unigine_dataset_test_1/mask_%d.jpg",
            m_activeCaptureIndex);
    }
}

void ImageGenerator::saveTextureAsync(const TexturePtr &texture, const char *pattern, int capture_index)
{
    if (!texture)
        return;

    Render::asyncTransferTextureToImage(
        MakeCallback([capture_index, pattern](ImagePtr image)
                     {
				if (image->getFormat() != Image::FORMAT_RGB8)
					image->convertToFormat(Image::FORMAT_RGB8);
				if (!Render::isFlipped())
					image->flipY();

				String full_name = String::format(pattern, capture_index);
				image->save(full_name.get());
				Log::message("ImageGenerator saved \"%s\"\n", full_name.get()); }),
        nullptr,
        texture);
}

void ImageGenerator::startCapture()
{
    if (m_state != STATE_IDLE)
        return;

    m_framesUntilCapture = capture_frequency.get();
    m_captureIndex = 0;
    m_activeCaptureIndex = 0;

    m_warmupCount = 0;
    m_state = STATE_WARMUP;
}

void ImageGenerator::stopCapture()
{
    if (m_state == STATE_IDLE)
        return;

    m_state = STATE_IDLE;
}
