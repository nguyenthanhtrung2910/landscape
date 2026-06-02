#pragma once
#include "InputInterface.h"
#include <UnigineComponentSystem.h>
#include "components/utils.h"

#define IMPL_INPUT_KEY(NAME)\
bool get##NAME##Key() const override\
{\
	for (const auto& i : inputs)\
	if (i->get##NAME##Key())\
	{\
		active_input = i;\
		return true;\
	}\
	return false;\
}\
\
Unigine::String get##NAME##KeyName() const override\
{\
	Unigine::String names = inputs[0]->get##NAME##KeyName();\
	for (int i =1; i < inputs.size(); i++)\
		names += " | " + inputs[i]->get##NAME##KeyName();\
	return names;\
}\
\
bool is##NAME##KeyDown() const override\
{\
	for (const auto& i : inputs)\
	if (i->is##NAME##KeyDown())\
	{\
		active_input = i;\
		return true;\
	}\
	return false;\
}\
\

#define IMPL_INPUT_AXIS(NAME)\
float get##NAME##Axis() const override\
{\
	float v = 0.f;\
	for (const auto &i : inputs)\
	{\
		if (Unigine::Math::abs(i->get##NAME##Axis()) > Unigine::Math::abs(v))\
		{\
			active_input = i;\
			v = i->get##NAME##Axis();\
		}\
		\
	}\
	return v;\
}\
\

class Drone;

class InputManager : public InputInterface
{
public:
	COMPONENT_DEFINE(InputManager, InputInterface);
	COMPONENT_INIT(init, INIT_ORDER::INPUT);
	COMPONENT_UPDATE(update, -1);
	COMPONENT_SHUTDOWN(shutdown);
	
	PROP_ARRAY(Node, inputs_param, "Inputs", "Tooltip", "Input Manager");
	
	IMPL_INPUT_AXIS(Throttle);
	IMPL_INPUT_AXIS(Yaw);
	IMPL_INPUT_AXIS(Pitch);
	IMPL_INPUT_AXIS(Roll);
	IMPL_INPUT_AXIS(CameraZoomIn);
	IMPL_INPUT_AXIS(CameraZoomOut);

#undef DECLARE_GET_METHOD_FLOAT

#define DECLARE_GET_METHOD_FLOAT(NAME)\
	float NAME() const override\
	{ float v = 0.0f;\
			for (const auto &i : inputs)\
	v = Unigine::Math::abs(v) > Unigine::Math::abs(i->NAME()) ? v : i->NAME();\
			return v; }

	DECLARE_GET_METHOD_FLOAT(getCameraYAxis);
	DECLARE_GET_METHOD_FLOAT(getCameraXAxis);

#undef DECLARE_GET_METHOD_FLOAT

	// keys
	IMPL_INPUT_KEY(ControlMode);
	IMPL_INPUT_KEY(ChangeCameraMode);

	IMPL_INPUT_KEY(NextCamera);

	IMPL_INPUT_KEY(CameraZoomReset);

	IMPL_INPUT_KEY(ResetCameraRot);

	IMPL_INPUT_KEY(FPVAutobalance);

	IMPL_INPUT_KEY(Restore);

	bool isStartLanding() const override;
	bool isStartFlight() const override;

	const Unigine::Map<Unigine::String, std::pair<Unigine::String, Unigine::String>>& getBinds() { return binds; };
	InputInterface const *getActiveInput() const { return active_input; }
	
private:
	void init();
	void update();
	void shutdown();

private:
	Unigine::Vector<InputInterface *> inputs;
	mutable InputInterface *active_input = nullptr;

	Drone *drone{nullptr};

	Unigine::WidgetVBoxPtr message_box;
	Unigine::WidgetLabelPtr message_label;

	Unigine::Map<Unigine::String, std::pair<Unigine::String, Unigine::String>> binds;
};

