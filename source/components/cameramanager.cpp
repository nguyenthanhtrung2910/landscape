#include "cameramanager.h"

#include <UnigineConsole.h>
#include <UnigineInput.h>
#include <UnigineGame.h>
#include <UnigineWorld.h>

REGISTER_COMPONENT(CameraManager);

using namespace Unigine;

void CameraManager::init()
{
    Vector<NodePtr> dummyCameras;
    World::getNodesByType(Node::PLAYER_DUMMY, dummyCameras);
    Vector<NodePtr> persecutorCameras;
    World::getNodesByType(Node::PLAYER_PERSECUTOR, persecutorCameras);
    for (const auto &node : dummyCameras)
    {
        m_cameras.push_back(checked_ptr_cast<PlayerDummy>(node));
    }
    for (const auto &node : persecutorCameras)
    {
        m_cameras.push_back(checked_ptr_cast<PlayerPersecutor>(node));
    }
    m_currentCamera = m_cameras.begin();
    WindowManager::getMainWindow()->getEventFuncEndRender().connect(this, &CameraManager::render);
}

void CameraManager::update()
{
    if (Input::isKeyDown(Input::KEY_G))
    {
        m_grabFlag = true;
    }
    if (Input::isKeyDown(Input::KEY_N))
    {
        m_currentCamera++;
        if (m_currentCamera == m_cameras.end())
        {
            m_currentCamera = m_cameras.begin();
        }
        Game::setPlayer(*m_currentCamera);
    }
}

void CameraManager::render()
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
