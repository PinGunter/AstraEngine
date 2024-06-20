#include <Scene.h>
#include <host_device.h>
#include <Device.h>
#include <nvvk/buffers_vk.hpp>
#include <Utils.h>
#include <obj_loader.h>

void Astra::Scene::createObjDescBuffer()
{
	if (_objDescBuffer.buffer != VK_NULL_HANDLE) {
		_alloc->destroy(_objDescBuffer);
	}

	nvvk::CommandPool cmdGen(AstraDevice.getVkDevice(), Astra::Device::getInstance().getGraphicsQueueIndex());

	auto cmdBuf = cmdGen.createCommandBuffer();
	if (!_objDesc.empty())
		_objDescBuffer = _alloc->createBuffer(cmdBuf, _objDesc, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

	cmdGen.submitAndWait(cmdBuf);
	_alloc->finalizeAndReleaseStaging();
}

Astra::Scene::Scene() : _transform(1.0f) {}

Astra::Scene& Astra::Scene::operator=(const Scene& s)
{
	_objModels = s._objModels;
	_instances = s._instances;
	_objDesc = s._objDesc;
	_textures = s._textures;
	_objDescBuffer = s._objDescBuffer;

	_lights = s._lights;
	_camera = s._camera;
	_transform = s._transform;

	return *this;
}

void Astra::Scene::loadModel(const std::string& filename, const glm::mat4& transform)
{
	// we cant load models until we have access to the resource allocator
	// if we have it, just create it
	// if we dont, postpone the operation to the init stage
	if (_alloc) {

		ObjLoader loader;
		loader.loadModel(filename);

		// TODO when having correct toVulkanMesh
		//Astra::Mesh mesh;
		//mesh.indices = loader.m_indices;
		//mesh.vertices = loader.m_vertices;
		//mesh.materials = loader.m_materials;
		//mesh.materialIndices = loader.m_matIndx;
		//mesh.textures = loader.m_textures;
		
		Astra::HostModel model;
		model.nbIndices = static_cast<uint32_t>(loader.m_indices.size());
		model.nbVertices = static_cast<uint32_t>(loader.m_vertices.size());

		// Create the buffers on Device and copy vertices, indices and materials
		nvvk::CommandPool  cmdBufGet(AstraDevice.getVkDevice(), AstraDevice.getGraphicsQueueIndex());
		VkCommandBuffer    cmdBuf = cmdBufGet.createCommandBuffer();
		VkBufferUsageFlags flag = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		VkBufferUsageFlags rayTracingFlags = flag | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		model.vertexBuffer = _alloc->createBuffer(cmdBuf, loader.m_vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | rayTracingFlags);
		model.indexBuffer = _alloc->createBuffer(cmdBuf, loader.m_indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | rayTracingFlags);
		model.matColorBuffer = _alloc->createBuffer(cmdBuf, loader.m_materials, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | rayTracingFlags);
		model.matIndexBuffer = _alloc->createBuffer(cmdBuf, loader.m_matIndx, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | rayTracingFlags);

		// Creates all textures found and find the offset for this model
		auto txtOffset = static_cast<uint32_t>(getTextures().size());
		AstraDevice.createTextureImages(cmdBuf, loader.m_textures, getTextures(), *_alloc);
		cmdBufGet.submitAndWait(cmdBuf);
		_alloc->finalizeAndReleaseStaging();

		// Creating information for device access
		ObjDesc desc{};
		desc.txtOffset = txtOffset;
		desc.vertexAddress = nvvk::getBufferDeviceAddress(AstraDevice.getVkDevice(), model.vertexBuffer.buffer);
		desc.indexAddress = nvvk::getBufferDeviceAddress(AstraDevice.getVkDevice(), model.indexBuffer.buffer);
		desc.materialAddress = nvvk::getBufferDeviceAddress(AstraDevice.getVkDevice(), model.matColorBuffer.buffer);
		desc.materialIndexAddress = nvvk::getBufferDeviceAddress(AstraDevice.getVkDevice(), model.matIndexBuffer.buffer);


		// Keeping the obj host model and device description
		addModel(model);
		addObjDesc(desc);

		// Keeping transformation matrix of the instance
		Astra::MeshInstance instance(static_cast<uint32_t>(getModels().size() - 1), transform);
		instance.setName(instance.getName() + " :: " + filename.substr(filename.size() - std::min(10, (int)filename.size() / 2), filename.size()));
		addInstance(instance);


		createObjDescBuffer();

	}
	else {
		_lazymodels.push_back(std::make_pair(filename, transform));
	}
}

void Astra::Scene::init(nvvk::ResourceAllocator* alloc)
{
	_alloc = (nvvk::ResourceAllocatorDma*)alloc;
	for (auto p : _lazymodels) {
		loadModel(p.first, p.second);
	}
	_lazymodels.clear();
}

void Astra::Scene::destroy() {

	_alloc->destroy(_objDescBuffer);

	for (auto& m : _objModels) {
		_alloc->destroy(m.vertexBuffer);
		_alloc->destroy(m.indexBuffer);
		_alloc->destroy(m.matColorBuffer);
		_alloc->destroy(m.matIndexBuffer);
	}

	for (auto& t : _textures) {
		_alloc->destroy(t);
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
	_lights.push_back(l);
}

void Astra::Scene::setCamera(CameraController* c)
{
	_camera = c;
}

void Astra::Scene::update()
{
	for (auto l : _lights) {
		l->update();
	}
	_camera->update();
	for (auto& i : _instances) {
		i.update();
	}
}

const std::vector<Astra::Light*>& Astra::Scene::getLights() const
{
	return _lights;
}

Astra::CameraController* Astra::Scene::getCamera() const
{
	return _camera;
}


std::vector<Astra::MeshInstance>& Astra::Scene::getInstances()
{
	return _instances;
}

std::vector<Astra::HostModel>& Astra::Scene::getModels()
{
	return _objModels;
}

std::vector<ObjDesc>& Astra::Scene::getObjDesc()
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
	Scene::init(alloc);
	//if (!_objDescBuffer.buffer) {
		//throw std::runtime_error("Empty Scene!");
	//}
	_rtBuilder.setup(AstraDevice.getVkDevice(), alloc, Astra::Device::getInstance().getGraphicsQueueIndex());
}

void Astra::SceneRT::update()
{
	for (auto l : _lights) {
		l->update();
	}
	_camera->update();
	std::vector<int> asupdates;
	for (int i = 0; i < _instances.size(); i++) {
		if (_instances[i].update()) {
			asupdates.push_back(i);
		}
	}

	for (int i : asupdates) {
		updateTopLevelAS(i);
	}

}

void Astra::SceneRT::createBottomLevelAS()
{
	if (getTLAS() != VK_NULL_HANDLE) {
		_rtBuilder.destroy();
		_asInstances.clear();
	}
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
	if (getTLAS() != VK_NULL_HANDLE) {
		_rtBuilder.destroy();
		_asInstances.clear();
	}
	_asInstances.reserve(_instances.size());
	for (const Astra::MeshInstance& inst : _instances) {
		VkAccelerationStructureInstanceKHR rayInst{};
		rayInst.transform = nvvk::toTransformMatrixKHR(inst.getTransform());
		rayInst.instanceCustomIndex = inst.getMeshIndex(); //gl_InstanceCustomIndexEXT
		rayInst.accelerationStructureReference = _rtBuilder.getBlasDeviceAddress(inst.getMeshIndex());
		rayInst.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FRONT_COUNTERCLOCKWISE_BIT_KHR;
		rayInst.mask = inst.getVisible() ? 0xFF : 0x00; // only be hit if raymask & instance.mask != 0
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

void Astra::SceneRT::destroy()
{
	Scene::destroy();
	_rtBuilder.destroy();
}
