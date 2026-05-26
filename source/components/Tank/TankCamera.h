#pragma once

#include "UnigineComponentSystem.h"
#include "UniginePlayers.h"

class TankCamera : public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(TankCamera, Unigine::ComponentBase);
	PROP_PARAM(Float, speed, 10.0f);
	COMPONENT_UPDATE(update);
	COMPONENT_INIT(init);

protected:
	void init();
	void update();
	void render();

private:
	Unigine::PlayerPersecutorPtr m_camera{nullptr};
	bool m_grabFlag{false};
	int m_captureIndex{0};
};