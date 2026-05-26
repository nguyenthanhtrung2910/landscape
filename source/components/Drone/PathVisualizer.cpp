#include "PathVisualizer.h"

REGISTER_COMPONENT(PathVisualizer);

using namespace Unigine;
using namespace Math;

void PathVisualizer::init()
{
	drone = ComponentSystem::get()->getComponentInWorld<Drone>();
	path_mesh = Mesh::create();
	path = ObjectMeshDynamic::create();
	on_reset();
}

void PathVisualizer::update()
{
	auto pos = drone->getPosition();

	if ((pos - prev_position).length2() < step)
		return;

	add_point(pos);
}

void PathVisualizer::shutdown()
{
	path.deleteLater();
}

void PathVisualizer::add_point(Vec3 position)
{
	Vec3 forward = normalize(position - prev_position);
	Vec3 right = normalize(cross(forward, Vec3_up));
	if (right.length2() < Consts::EPS)
		right = normalize(cross(Vec3_forward, forward));
	Vec3 up = normalize(cross(forward, right));

	path->addVertex(vec3(prev_position - up * 0.01f));
	path->addVertex(vec3(prev_position + up * 0.01f));
	path->addVertex(vec3(prev_position - right * 0.01f));
	path->addVertex(vec3(prev_position + right * 0.01f));
	auto last_index = path->getNumVertex() - 1;
	path->addIndex(last_index - 5);
	path->addIndex(last_index - 4);
	path->addIndex(last_index);
	path->addIndex(last_index - 5);
	path->addIndex(last_index);
	path->addIndex(last_index - 1);
	last_index -= 2;
	path->addIndex(last_index - 5);
	path->addIndex(last_index - 4);
	path->addIndex(last_index);
	path->addIndex(last_index - 5);
	path->addIndex(last_index);
	path->addIndex(last_index - 1);

	path->updateBounds();
	path->flushVertex();
	path->flushIndices();

	prev_position = position;
}

void PathVisualizer::on_reset()
{
	prev_position = drone->getPosition();

	path->clearIndices();
	path->clearVertex();

	path->addVertex(vec3(prev_position - drone->getNode()->getWorldTransform().getAxisZ() * 0.01f));
	path->addVertex(vec3(prev_position + drone->getNode()->getWorldTransform().getAxisZ() * 0.01f));
	path->addVertex(vec3(prev_position - drone->getNode()->getWorldTransform().getAxisX() * 0.01f));
	path->addVertex(vec3(prev_position + drone->getNode()->getWorldTransform().getAxisX() * 0.01f));

	for (int i = 0; i < path->getNumSurfaces(); i++)
	{
		path->setMaterial(path_material, i);
		path->setSurfaceProperty("surface_base", i);
		path->setCastShadow(false, i);
		path->setCastWorldShadow(false, i);
		path->setIntersection(false, i);
		path->setCollision(false, i);
	}

	path->flushVertex();
	path->flushIndices();
}
