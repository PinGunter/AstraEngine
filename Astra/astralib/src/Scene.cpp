#include <Scene.h>
#include <host_device.h>

Astra::Scene::Scene() : _transform(1.0f) {}

void Astra::Scene::addNode(Node3D* n)
{
	_nodes.push_back(n);
}

void Astra::Scene::removeNode(const Node3D& n)
{
	auto eraser = _nodes.begin();
	bool found = false;
	for (auto it = _nodes.begin(); it != _nodes.end() && !found; ++it) {
		if (*(*it) == n) {
			eraser = it;
			found = true;
		}
	}
	if (found)	_nodes.erase(eraser);
}

void Astra::Scene::addLight(Light* l)
{
	// since currently we only have 1
	_light = l;
}

void Astra::Scene::setCamera(CameraController* c)
{
	_camera = c;
}

void Astra::Scene::setTLAS(VkAccelerationStructureKHR tlas)
{
	_tlas = tlas;
}

Astra::Light* Astra::Scene::getLight() const
{
	return _light;
}

Astra::CameraController* Astra::Scene::getCamera() const
{
	return _camera;
}

VkAccelerationStructureKHR Astra::Scene::getTLAS() const
{
	return _tlas;
}

glm::mat4& Astra::Scene::getTransform()
{
	return _transform;
}
