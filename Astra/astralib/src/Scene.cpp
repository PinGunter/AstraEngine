#include <Scene.h>
#include <host_device.h>
#include <Device.h>
#include <nvvk/buffers_vk.hpp>

Astra::Scene::Scene() : _transform(1.0f) {}

void Astra::Scene::destroy() {
	auto& alloc = Astra::Device::getInstance().getResAlloc();

	alloc.destroy(_objDescBuffer);

	for (auto& m : _objModels) {
		alloc.destroy(m.vertexBuffer);
		alloc.destroy(m.indexBuffer);
		alloc.destroy(m.matColorBuffer);
		alloc.destroy(m.matIndexBuffer);
	}

	for (auto& t : _textures) {
		alloc.destroy(t);
	}
}

void Astra::Scene::addModel(const HostModel& model)
{
	_objModels.push_back(model);
}

void Astra::Scene::addInstance(const MeshInstance& instance)
{
	_instances.push_back(instance);
}

void Astra::Scene::addObjDesc(const ObjDesc& objdesc)
{
	_objDesc.push_back(objdesc);
}

void Astra::Scene::removeNode(const MeshInstance& n)
{
	auto eraser = _instances.begin();
	bool found = false;
	for (auto it = _instances.begin(); it != _instances.end() && !found; ++it) {
		if ((*it) == n) {
			eraser = it;
			found = true;
		}
	}
	if (found)	_instances.erase(eraser);
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

const std::vector<Astra::MeshInstance>& Astra::Scene::getInstances() const
{
	return _instances;
}

const std::vector<Astra::HostModel>& Astra::Scene::getModels() const
{
	return _objModels;
}

const std::vector<ObjDesc>& Astra::Scene::getObjDesc() const
{
	return _objDesc;
}

std::vector<nvvk::Texture>& Astra::Scene::getTextures()
{
	return _textures;
}

nvvk::Buffer& Astra::Scene::getObjDescBuff()
{
	return _objDescBuffer;
}

glm::mat4& Astra::Scene::getTransformRef()
{
	return _transform;
}

const glm::mat4& Astra::Scene::getTransformRef() const
{
	return _transform;
}

void Astra::Scene::updatePushConstantRaster(PushConstantRaster& pc)
{
	// TODO, futurure
}

void Astra::Scene::updatePushConstant(PushConstantRay& pc)
{
	// TODO, futurure
}
