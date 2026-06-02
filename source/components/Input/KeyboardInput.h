#pragma once
#include <UnigineInterface.h>
#include "InputInterface.h"
#include "components/utils.h"

#define IMPL_INPUT_AXIS(NAME_MINOR, NAME_MAJOR)                                       \
private:                                                                                           \
	AxisBind NAME_MINOR##_bind;                                                                    \
                                                                                                   \
public:                                                                                            \
	PROP_STRUCT(KeyboardAxis, NAME_MINOR##_param, #NAME_MAJOR, "", "Keyboard Input");                  \
	float get##NAME_MAJOR##Axis() const override                                                   \
	{                                                                                              \
		return NAME_MINOR##_bind.getCurrentValue();                                                \
	}


#define IMPL_INPUT_KEY(NAME_MINOR, NAME_MAJOR)                                        \
private:                                                                                           \
	KeyBind NAME_MINOR##_bind;                                                                     \
                                                                                                   \
public:                                                                                            \
	PROP_STRUCT(KeyboardKey, NAME_MINOR##_param, #NAME_MAJOR, "", "Keyboard Input");               \
	bool get##NAME_MAJOR##Key() const override                                                     \
	{                                                                                              \
		return NAME_MINOR##_bind.getCurrentValue();                                                \
	}                                                                                              \
	bool is##NAME_MAJOR##KeyDown() const override                                                  \
	{                                                                                              \
		return NAME_MINOR##_bind.isKeyDown();                                                      \
	}                                                                                              \
	Unigine::String get##NAME_MAJOR##KeyName() const override                                      \
	{                                                                                              \
		return NAME_MINOR##_bind.getKeyName();                                                     \
	}


class KeyboardInput : public InputInterface
{
public:
	COMPONENT(KeyboardInput, InputInterface);
	COMPONENT_INIT(init, INIT_ORDER::INPUT - 1);
	COMPONENT_UPDATE(update);
	COMPONENT_SHUTDOWN(shutdown);

	PROP_NAME("KeyboardInput");

	// structs for editor settings
	struct KeyboardKey: public Unigine::ComponentStruct
	{
		PROP_KEYBOARD_KEY(key_param, "Key");
		PROP_PARAM(Toggle, default_state, false);
	};

	struct KeyboardAxis: public Unigine::ComponentStruct
	{
		PROP_AXIS_CLAMP(clamp_param);
		PROP_PARAM(Toggle, inverse_param, 0, "Inverse");
		PROP_KEYBOARD_KEY(positive_key_param, "Positive Key");
		PROP_KEYBOARD_KEY(negative_key_param, "Negative Key");
		PROP_PARAM(Float, dead_zone_param, 0.01f, "Dead Zone");
		PROP_PARAM(Float, acceleration_param, 10.0f, "Acceleration");
		PROP_PARAM(Float, gravity_param, 10.0f, "Gravity");
	};

private:
	// structs for internal update
	struct AxisBind
	{
		Unigine::Input::KEY positive_key = Unigine::Input::KEY::KEY_UNKNOWN;
		Unigine::Input::KEY negative_key = Unigine::Input::KEY::KEY_UNKNOWN;

		int axis_clamp = 0;
		int axis_inverse = 0;
		float dead = 0.0f;
		float acceleration = 1.0f;
		float gravity = 1.0f;

		bool isValid() const { return positive_key >= 0 && negative_key >= 0; }
		void set(InputInterface* input, const KeyboardAxis &axis_param, Unigine::String description);
		void update(float ifps);
		float getCurrentValue() const;
		void setValueForce(float value) { current_value = value; }
		void clear() { current_value = 0.0f; }

	private:
		float current_value = 0.0f;
	};

	struct KeyBind
	{
		void set(InputInterface* input, const KeyboardKey &key_param, Unigine::String description);
		void update();
		bool getCurrentValue() const;
		void setValueForce(bool value) { current_value = value; }
		void clear() { current_value = false; }
		bool isValid() const { return key >= 0; }
		bool isKeyDown() const { return is_key_down; }
		Unigine::String getKeyName() const { return Unigine::Input::getKeyName(key); }

		Unigine::Input::KEY key = Unigine::Input::KEY::KEY_UNKNOWN;
		bool current_value = false;
		bool is_key_down = false;

		mutable bool is_once_updated = false;
		bool default_value = false;
	};


public:

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
};
