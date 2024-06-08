#include <Scene.h>
#include <host_device.h>
#include <Device.h>
#include <nvvk/buffers_vk.hpp>

Astra::Scene::Scene() : _transform(1.0f) {}

Astra::Scene& Astra::Scene::operator=(const Scene& s)
{
	_objModels = s._objModels;
	_instances = s._instances;
	_objDesc = s._objDesc;
	_textures = s._textures;
	_objDescBuffer = s._objDescBuffer;

	_light = s._light;
	_camera = s._camera;
	_transform = s._transform;

	return *this;
}

void Astra::Scene::destroy(nvvk::ResourceAllocator* alloc) {

	alloc->destroy(_objDescBuffer);

	for (auto& m : _objModels) {
		alloc->destroy(m.vertexBuffer);
		alloc->destroy(m.indexBuffer);
		alloc->destroy(m.matColorBuffer);
		alloc->destroy(m.matIndexBuffer);
	}

	for (auto& t : _textures) {
		alloc->destroy(t);
	}

	for (auto& m : _instances) {
		m.destroy();
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

Astra::Light* Astra::Scene::getLight() const
{
	return _light;
}

Astra::CameraController* Astra::Scene::getCamera() const
{
	return _camera;
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

// rt scene

void Astra::SceneRT::init(nvvk::ResourceAllocator* alloc)
{
	_rtBuilder.setup(AstraDevice.getVkDevice(), alloc, Astra::Device::getInstance().getGraphicsQueueIndex());
}

void Astra::SceneRT::createBottomLevelAS()
{
	std::vector<nvvk::RaytracingBuilderKHR::BlasInput> allBlas;
	allBlas.reserve(_objModels.size());

	for (const auto& obj : _objModels) {
		auto blas = AstraDevice.objectToVkGeometry(obj);

		allBlas.emplace_back(blas);
	}

	_rtBuilder.buildBlas(allBlas, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
}

void Astra::SceneRT::createTopLevelAS()
{
	_asInstances.reserve(_instances.size());
	for (const Astra::MeshInstance& inst : _instances) {
		VkAccelerationStructureInstanceKHR rayInst{};
		rayInst.transform = nvvk::toTransformMatrixKHR(inst.getTransform());
		rayInst.instanceCustomIndex = inst.getMeshIndex(); //gl_InstanceCustomIndexEXT
		rayInst.accelerationStructureReference = _rtBuilder.getBlasDeviceAddress(inst.getMeshIndex());
		rayInst.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FRONT_COUNTERCLOCKWISE_BIT_KHR;
		rayInst.mask = 0xFF; // only be hit if raymask & instance.mask != 0
		rayInst.instanceShaderBindingTableRecordOffset = 0; // the same hit group for all objects

		_asInstances.emplace_back(rayInst);
	}
	_rtBuilder.buildTlas(_asInstances, VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
}

void Astra::SceneRT::updateTopLevelAS(int instance_id)
{
	const auto& inst = _instances[instance_id];
	VkAccelerationStructureInstanceKHR rayInst{};
	rayInst.transform = nvvk::toTransformMatrixKHR(inst.getTransform());
	rayInst.instanceCustomIndex = inst.getMeshIndex(); //gl_InstanceCustomIndexEXT
	rayInst.accelerationStructureReference = _rtBuilder.getBlasDeviceAddress(inst.getMeshIndex());
	rayInst.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FRONT_COUNTERCLOCKWISE_BIT_KHR;
	rayInst.mask = inst.getVisible() ? 0xFF : 0x00; // only be hit if raymask & instance.mask != 0
	rayInst.instanceShaderBindingTableRecordOffset = 0; // the same hit group for all objects
	_asInstances[instance_id] = rayInst;

	_rtBuilder.buildTlas(_asInstances, VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR, true);
}

VkAccelerationStructureKHR Astra::SceneRT::getTLAS() const
{
	return _rtBuilder.getAccelerationStructure();
}

void Astra::SceneRT::destroy(nvvk::ResourceAllocator * alloc)
{
	Scene::destroy(alloc);
	_rtBuilder.destroy();
}
