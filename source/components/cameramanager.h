#pragma once

#include <vector>

#include <UnigineComponentSystem.h>

class CameraManager : public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(CameraManager, Unigine::ComponentBase);
	COMPONENT_INIT(init);
	COMPONENT_UPDATE(update);

private:
	void init();
	void update();
	void render();

private:
	std::vector<Unigine::PlayerPtr> m_cameras;
	std::vector<Unigine::PlayerPtr>::iterator m_currentCamera;
	bool m_grabFlag{false};
	int m_captureIndex{0};
};
