#include "UnigineConsole.h"
#include "UnigineEngine.h"
#include "UnigineInput.h"
#include "UnigineGame.h"
#include "UniginePlayers.h"
#include "TankCamera.h"

REGISTER_COMPONENT(TankCamera);

using namespace Unigine;

void TankCamera::init()
{
    m_camera = checked_ptr_cast<PlayerPersecutor>(node);
    WindowManager::getMainWindow()->getEventFuncEndRender().connect(this, &TankCamera::render);
}

void TankCamera::update()
{
    float ifps = Game::getIFps();
    if (Input::isKeyPressed(Input::KEY_I))
    {
        m_camera->worldTranslate((Math::Vec3)m_camera->getViewDirection() * speed * ifps);
        return;
    }
    if (Input::isKeyPressed(Input::KEY_K))
    {
        m_camera->worldTranslate((Math::Vec3)m_camera->getViewDirection() * (-speed) * ifps);
        return;
    }
    if (Input::isKeyDown(Input::KEY_G))
    {
        m_grabFlag = true;
        return;
    }
    if (Input::isKeyDown(Input::KEY_O))
    {
        Game::setPlayer(m_camera);
        return;
    }
}

void TankCamera::render()
{
    // Skip capture unless grab() was called
    if (!m_grabFlag)
    {
        return;
    }
    m_grabFlag = false;

    // Get a temporary texture matching window dimensions for the capture
    TexturePtr temporary_texture = Render::getTemporaryTexture2D(1920, 1080, Texture::FORMAT_RGBA8, 0);

    // Copy current color buffer contents into our temporary texture
    temporary_texture->copy2D();

    // Transfer texture data to CPU image asynchronously (non-blocking)
    int index{m_captureIndex++};
    Render::asyncTransferTextureToImage(nullptr, MakeCallback([index](ImagePtr image)
                                                              {
        if (image->getFormat() != Image::FORMAT_RGB8)
            image->convertToFormat(Image::FORMAT_RGB8);
		// Some graphics APIs render upside-down; flip if needed for correct orientation
		if (!Render::isFlipped())
		{
			image->flipY();
		}
        String full_name = String::format("screenshot_%d.jpg", index);
		image->save(full_name);
		Log::message("Screenshot saved to \"data/%s\"\n", full_name.get()); }),
                                        temporary_texture);

    // Return texture to pool for reuse
    Render::releaseTemporaryTexture(temporary_texture);
}
