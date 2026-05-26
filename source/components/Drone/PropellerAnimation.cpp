#include "PropellerAnimation.h"

#include <UnigineGame.h>
#include <UnigineNodes.h>
#include <UnigineObjects.h>

REGISTER_COMPONENT(PropellerAnimation);

using namespace Unigine;
using namespace Math;

void PropellerAnimation::setShutterEnabled(bool enabled)
{
	shutter_enabled = enabled;
	shutter_object->setEnabled(shutter_enabled);
	dynamic_object->setEnabled(!shutter_enabled);
}

void PropellerAnimation::init()
{
	// drone = getComponentInParent<Drone>(node);
	// CHECK_COMPONENT_VALUE(drone, "can not find Drone component in parent");

	// dynamic
	dynamic_object = checked_ptr_cast<Object>(dynamic_node.get());
	CHECK_COMPONENT_VALUE(dynamic_object, "can not find dynamic_object");

	dynamic_fl_surface_id = dynamic_object->findSurface(dynamic_fl_surface_name.get());
	CHECK_COMPONENT_VALUE((dynamic_fl_surface_id != -1), "can not find dynamic_fl_surface_name surface");
	dynamic_fr_surface_id = dynamic_object->findSurface(dynamic_fr_surface_name.get());
	CHECK_COMPONENT_VALUE((dynamic_fr_surface_id != -1), "can not find dynamic_fr_surface_name surface");

	// static
	ObjectPtr tmp;
	tmp = checked_ptr_cast<Object>(static_fl.get());
	CHECK_COMPONENT_VALUE(tmp, "can not find static_fl object");
	static_propellers.append(tmp);

	tmp = checked_ptr_cast<Object>(static_fr.get());
	CHECK_COMPONENT_VALUE(tmp, "can not find static_fr object");
	static_propellers.append(tmp);

	tmp = checked_ptr_cast<Object>(static_bl.get());
	CHECK_COMPONENT_VALUE(tmp, "can not find static_bl object");
	static_propellers.append(tmp);

	tmp = checked_ptr_cast<Object>(static_br.get());
	CHECK_COMPONENT_VALUE(tmp, "can not find static_br object");
	static_propellers.append(tmp);

	// shutter
	shutter_object = checked_ptr_cast<Object>(shutter_node.get());
	CHECK_COMPONENT_VALUE(shutter_object, "can not find shutter_object");

	shutter_fl_surface_id = shutter_object->findSurface(shutter_surface_name_fl.get());
	CHECK_COMPONENT_VALUE((shutter_fl_surface_id != -1), "can not find shutter_surface_name_fl surface");
	shutter_fr_surface_id = shutter_object->findSurface(shutter_surface_name_fr.get());
	CHECK_COMPONENT_VALUE((shutter_fr_surface_id != -1), "can not find shutter_surface_name_fr surface");

	auto shutter_material = shutter_object->getMaterialInherit(shutter_fl_surface_id);

	// set static on start
	reset();

	// drone->addResetCallback(MakeCallback(this, &PropellerAnimation::reset));
	// drone->addChangeStateCallback(MakeCallback(this, &PropellerAnimation::on_change_state));
	// drone->getCamera()->addOnCameraChangedCallback(MakeCallback(this, &PropellerAnimation::on_change_camera));
}

void PropellerAnimation::update()
{
	// if (drone->getState() == CRASHED)
	// 	return;

	// float fl_pitch = drone->getPropellerNormalRPM(0);
	// float fr_pitch = drone->getPropellerNormalRPM(1);
	// float br_pitch = drone->getPropellerNormalRPM(2);
	// float bl_pitch = drone->getPropellerNormalRPM(3);

	float average = 0.5f;

	float angle = average * static_speed_multiplyier;

	// rotate separetly
	for (auto &it : static_propellers)
		it->rotate(0, 0, angle);

	// rotate dynamic
	dynamic_object->setMaterialParameterFloat(dynamic_speed_material_param.get(), average, dynamic_fl_surface_id);
	dynamic_object->setMaterialParameterFloat(dynamic_speed_material_param.get(), average, dynamic_fr_surface_id);

	// rotate shutter
	shutter_object->setMaterialParameterFloat(shutter_speed_material_param.get(), average, shutter_fl_surface_id);
	shutter_object->setMaterialParameterFloat(shutter_speed_material_param.get(), average, shutter_fr_surface_id);

	set_fade(Utils::rerange(0.0f, 1.0f, 0.05f, 0.1f, average));

	// TODO make the propellers rotate separately
}

void PropellerAnimation::shutdown()
{
}

void PropellerAnimation::set_fade(float v)
{
	v = saturate(v);

	for (auto &it : static_propellers)
	{
		it->setEnabled(v != 1.0f);
		it->setMaterialParameterFloat(static_transparent_material_param.get(), 1 - v, 0);
	}
	dynamic_object->setMaterialParameterFloat(static_transparent_material_param.get(), v, dynamic_fl_surface_id);
	dynamic_object->setMaterialParameterFloat(static_transparent_material_param.get(), v, dynamic_fr_surface_id);
	shutter_object->setMaterialParameterFloat(shutter_transparent_material_param.get(), v, shutter_fl_surface_id);
	shutter_object->setMaterialParameterFloat(shutter_transparent_material_param.get(), v, shutter_fr_surface_id);
}

void PropellerAnimation::reset()
{
	set_fade(0);
	setShutterEnabled(shutter_enabled);
}
