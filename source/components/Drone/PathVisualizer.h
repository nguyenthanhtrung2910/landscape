#pragma once

#include <UnigineComponentSystem.h>
#include "Drone.h"

class PathVisualizer : public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(PathVisualizer, Unigine::ComponentBase);

	COMPONENT_INIT(init, INIT_ORDER::DRONE + 1);
	COMPONENT_UPDATE(update);
	COMPONENT_SHUTDOWN(shutdown);

	PROP_PARAM(Material, path_material);
	PROP_PARAM(Float, step, 0.01f);

	void setEnabled(bool enable) { path->setEnabled(enable); }

private:
	void init();
	void update();
	void shutdown();

	void add_point(Unigine::Math::Vec3 position);

	void on_reset();
	void* reset_callback_id = nullptr;

	Unigine::ObjectMeshDynamicPtr path;
	Unigine::MeshPtr path_mesh;

	Unigine::Math::Vec3 prev_position;
	Drone *drone;
};