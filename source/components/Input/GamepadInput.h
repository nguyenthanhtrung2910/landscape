#pragma once
#include <UnigineInterface.h>
#include "InputInterface.h"
#include "InputUtils.h"
#include "components/utils.h"

#define IMPL_INPUT_KEY(NAME_MINOR, NAME_MAJOR)                                                             \
private:                                                                                                   \
	KeyBind NAME_MINOR##_bind;                                                                             \
                                                                                                           \
public:                                                                                                    \
	PROP_STRUCT(GamepadKey, NAME_MINOR##_param, #NAME_MAJOR, "", "Gamepad Input")  \
	bool get##NAME_MAJOR##Key() const override                                                             \
	{                                                                                                      \
		return NAME_MINOR##_bind.getCurrentValue();                                                        \
	}                                                                                                      \
	bool is##NAME_MAJOR##KeyDown() const override                                                          \
	{                                                                                                      \
		return NAME_MINOR##_bind.isButtonDown();                                                           \
	}                                                                                                      \
	Unigine::String get##NAME_MAJOR##KeyName() const override                                              \
	{                                                                                                      \
		return NAME_MINOR##_bind.getKeyName();                                                             \
	}


#define IMPL_INPUT_AXIS(NAME_MINOR, NAME_MAJOR)                                                                            \
private:                                                                                                                   \
	AxisBind NAME_MINOR##_bind;                                                                                            \
                                                                                                                           \
public:                                                                                                                    \
	PROP_STRUCT(GamepadAxis, NAME_MINOR##_param,  #NAME_MAJOR, "", "Gamepad Input")                \
	float get##NAME_MAJOR##Axis() const override                                                                           \
	{                                                                                                                      \
		return NAME_MINOR##_bind.getCurrentValue();                                                                        \
	}


class GamepadInput : public InputInterface
{
public:
	struct GamepadKey : public Unigine::ComponentStruct
	{
		PROP_GAMEPAD_KEY(key_param, "Key");
	};

	struct GamepadAxis : public Unigine::ComponentStruct
	{
		PROP_AXIS_CLAMP(clamp_param);
		PROP_PARAM(Toggle, inverse_param, 0, "Inverse");
		PROP_GAMEPAD_AXIS(axis_param);
		PROP_PARAM(Float, dead_zone_param, 0.01f, "Dead zone");
		PROP_PARAM(Toggle, use_fake_axis_param, 0, "Use Fake Axis");
		PROP_GAMEPAD_KEY(positive_key_param, "Key positive");
		PROP_GAMEPAD_KEY(negative_key_param, "Key negative");
		
		PROP_PARAM(Float, sensitivity_param, 10.0f, "Sensitivity");
	};

	struct GamepadStick : public Unigine::ComponentStruct
	{
		PROP_PARAM(Toggle, enabled);
		PROP_PARAM(Switch, type, 0, "LEFT,RIGHT");
		PROP_PARAM(Float, origin_y, 0.0f);

		void update(float ifps, const Unigine::InputGamePadPtr &gamepad);
		float getSignY() const { return current_value.y < origin_y ? -1.0f : 1.0f; }

		Unigine::Math::vec2 current_value;
		float current_length = 0.0f;
	};

private:
	struct KeyBind
	{
		int key = 0;

		void set(InputInterface* input, const GamepadKey &key_param, Unigine::String description);
		void update(const Unigine::InputGamePadPtr &gamepad);
		bool getCurrentValue() const;
		bool isValid() const { return key >= 0; }
		bool isButtonDown() const { return is_button_down; }
		Unigine::String getKeyName() const { return getGamepadButtonName(key); }

	private:
		bool current_value = false;
		bool is_button_down = false;
	};

	struct AxisBind
	{
		int axis = 0;

		int axis_clamp = 0;
		int axis_inverse = 0;
		float dead = 0.0f;

		bool use_fake_axis = false;
		int positive_key = 0;
		int negative_key = 0;
		float sensitivity = 0.0f;

		void set(InputInterface* input, const GamepadAxis &axis_param, Unigine::String description);
		void update(float ifps, const Unigine::InputGamePadPtr &gamepad);
		float getCurrentValue() const;
		bool isValid() const { return positive_key >= 0 && negative_key >= 0; }

	private:
		float current_value = 0.0f;
	};

public:
	COMPONENT(GamepadInput, InputInterface)
	COMPONENT_INIT(init, INIT_ORDER::INPUT - 1)
	COMPONENT_UPDATE(update)
	COMPONENT_SHUTDOWN(shutdown)

	PROP_NAME("GamepadInput")

	// axes
	IMPL_INPUT_AXIS(throttle, Throttle)
	IMPL_INPUT_AXIS(yaw, Yaw)
	IMPL_INPUT_AXIS(pitch, Pitch)
	IMPL_INPUT_AXIS(roll, Roll)
	IMPL_INPUT_AXIS(camera_zoom_in, CameraZoomIn)
	IMPL_INPUT_AXIS(camera_zoom_out, CameraZoomOut)
	IMPL_INPUT_AXIS(camera_axis_y, CameraY)
	IMPL_INPUT_AXIS(camera_axis_x, CameraX)

	// keys
	IMPL_INPUT_KEY(control_mode, ControlMode)
	IMPL_INPUT_KEY(change_camera_mode, ChangeCameraMode)
	IMPL_INPUT_KEY(next_camera, NextCamera)
	IMPL_INPUT_KEY(camera_zoom_reset, CameraZoomReset)
	IMPL_INPUT_KEY(fpv_auto_balance, FPVAutobalance)
	IMPL_INPUT_KEY(reset_camera_rot, ResetCameraRot)
	IMPL_INPUT_KEY(restore, Restore)

	bool isStartLanding() const override;
	bool isStartFlight() const override;

private:
	void init();
	void update();
	void shutdown(); 

	void update_axis_bind(AxisBind &axis_bind, float ifps);
	void update_key_bind(KeyBind &key_bind);

	Unigine::Vector<AxisBind *> axis_binds;
	Unigine::Vector<KeyBind *> key_binds;

	Unigine::InputGamePadPtr gamepad;
};

