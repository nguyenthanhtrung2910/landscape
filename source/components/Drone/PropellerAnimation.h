#pragma once

#include <UnigineComponentSystem.h>
#include "Utils.h"

class Drone;

class PropellerAnimation : public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(PropellerAnimation, Unigine::ComponentBase);
	COMPONENT_INIT(init, INIT_ORDER::DRONE + 1);
	COMPONENT_UPDATE(update);
	COMPONENT_SHUTDOWN(shutdown);

	PROP_GROUP("Dynamic");
	PROP_PARAM(Node, dynamic_node);
	PROP_PARAM(String, dynamic_fl_surface_name, "fl");
	PROP_PARAM(String, dynamic_fr_surface_name, "fr");
	PROP_PARAM(String, dynamic_speed_material_param, "normal_speed");
	PROP_PARAM(String, dynamic_transparent_material_param, "transparent");

	PROP_GROUP("Static");
	PROP_PARAM(Node, static_fl);
	PROP_PARAM(Node, static_fr);
	PROP_PARAM(Node, static_bl);
	PROP_PARAM(Node, static_br);
	PROP_PARAM(Float, static_speed_multiplyier, 500);
	PROP_PARAM(String, static_transparent_material_param, "transparent");

	PROP_GROUP("Shutter");
	PROP_PARAM(Node, shutter_node);
	PROP_PARAM(String, shutter_surface_name_fl, "fl");
	PROP_PARAM(String, shutter_surface_name_fr, "fr");
	PROP_PARAM(String, shutter_speed_material_param, "normal_speed");
	PROP_PARAM(String, shutter_transparent_material_param, "transparent");

	void setShutterEnabled(bool enabled);
	bool isShutterEnabled() const { return shutter_enabled; }

private:
	void init();
	void update();
	void shutdown();

	void set_fade(float v);
	void reset();

	Unigine::ObjectPtr dynamic_object;
	int dynamic_fl_surface_id{-1};
	int dynamic_fr_surface_id{-1};

	Unigine::Vector<Unigine::ObjectPtr> static_propellers;

	Unigine::ObjectPtr shutter_object;
	int shutter_fl_surface_id{-1};
	int shutter_fr_surface_id{-1};

	// Drone *drone{nullptr};
	bool shutter_enabled{false};
};
