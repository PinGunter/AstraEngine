#include <Scene.h>
#include <host_device.h>
#include <Device.h>
#include <nvvk/buffers_vk.hpp>
#include <Utils.h>

void Astra::Scene::createObjDescBuffer()
{
	if (_objDescBuffer.buffer != VK_NULL_HANDLE)
	{
		_alloc->destroy(_objDescBuffer);
	}

	nvvk::CommandPool cmdGen(AstraDevice.getVkDevice(), AstraDevice.getGraphicsQueueIndex());

	auto cmdBuf = cmdGen.createCommandBuffer();
	std::vector<ObjDesc> objDescs;
	for (auto& mesh : _objModels)
	{
		objDescs.push_back(mesh.descriptor);
	}
	if (!objDescs.empty())
		_objDescBuffer = _alloc->createBuffer(cmdBuf, objDescs, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

	cmdGen.submitAndWait(cmdBuf);
	_alloc->finalizeAndReleaseStaging();
}

void Astra::Scene::createCameraUBO()
{
	_cameraUBO = AstraDevice.createUBO<CameraUniform>(_alloc);
}

void Astra::Scene::updateCameraUBO(const CommandList& cmdList)
{
	AstraDevice.updateUBO<CameraUniform>(_camera->getCameraUniform(), _cameraUBO, cmdList);
}

void Astra::Scene::createLightsUBO()
{
	_lightsUBO = AstraDevice.createUBO<LightsUniform>(_alloc);
}

void Astra::Scene::updateLightsUBO(const CommandList& cmdList)
{
	AstraDevice.updateUBO<LightsUniform>(_lightsUniform, _lightsUBO, cmdList);
}

void Astra::Scene::loadModel(const std::string& filename, const glm::mat4& transform)
{
	// we cant load models until we have access to the resource allocator
	// if we have it, just create it
	// if we dont, postpone the operation to the init stage
	if (_alloc)
	{
		// find the offset for this model
		auto txtOffset = static_cast<uint32_t>(getTextures().size());
		// allocating cmdbuffers
		nvvk::CommandPool cmdBufGet(AstraDevice.getVkDevice(), AstraDevice.getGraphicsQueueIndex());
		VkCommandBuffer cmdBuf = cmdBufGet.createCommandBuffer();
		Astra::CommandList cmdList(cmdBuf);

		Astra::Mesh mesh;
		mesh.loadFromFile(filename);
		mesh.meshId = getModels().size();

		// color space to linear
		for (auto& m : mesh.materials)
		{
			m.ambient = glm::pow(m.ambient, glm::vec3(2.2f));
			m.diffuse = glm::pow(m.diffuse, glm::vec3(2.2f));
			m.specular = glm::pow(m.specular, glm::vec3(2.2f));
		}

		// creates the buffers and descriptors neeeded
		mesh.create(cmdList, _alloc, txtOffset);

		// Creates all textures found
		//AstraDevice.createTextureImages(cmdBuf, mesh.textures, getTextures(), *_alloc);
		for (auto p : mesh.textures) {
			getTextures().push_back(AstraDevice.createTextureImage(cmdList, p, *_alloc));
		}
		if (mesh.textures.empty() && getTextures().empty()) getTextures().push_back(AstraDevice.createTextureImage(cmdList, "", *_alloc, true));

		cmdBufGet.submitAndWait(cmdBuf);
		_alloc->finalizeAndReleaseStaging();

		// adds the model to the scene
		addModel(mesh);

		// creates an instance of the model
		Astra::MeshInstance instance(mesh.meshId, transform);
		instance.setName(instance.getName() + " :: " + filename.substr(filename.size() - std::min(10, (int)filename.size() / 2 - 4), filename.size()));
		addInstance(instance);

		// creates the descriptor buffer
		createObjDescBuffer();
	}
	else
	{
		_lazymodels.push_back(std::make_pair(filename, transform));
	}
}

void Astra::Scene::init(nvvk::ResourceAllocator* alloc)
{
	_alloc = (nvvk::ResourceAllocatorDma*)alloc;
	for (auto& p : _lazymodels)
	{
		loadModel(p.first, p.second);
	}
	_lazymodels.clear();
	createCameraUBO();
	createLightsUBO();
}

void Astra::Scene::destroy()
{

	_alloc->destroy(_objDescBuffer);

	for (auto& m : _objModels)
	{
		_alloc->destroy(m.vertexBuffer);
		_alloc->destroy(m.indexBuffer);
		_alloc->destroy(m.matColorBuffer);
		_alloc->destroy(m.matIndexBuffer);
	}

	for (auto& t : _textures)
	{
		_alloc->destroy(t);
	}

	for (auto& m : _instances)
	{
		m.destroy();
	}

	_alloc->destroy(_cameraUBO);
	_alloc->destroy(_lightsUBO);
}

void Astra::Scene::addModel(const Astra::Mesh& model)
{
	_objModels.push_back(model);
}

void Astra::Scene::addInstance(const MeshInstance& instance)
{
	_instances.push_back(instance);
}

void Astra::Scene::removeInstance(const MeshInstance& n)
{
	auto eraser = _instances.begin();
	bool found = false;
	for (auto it = _instances.begin(); it != _instances.end() && !found; ++it)
	{
		if ((*it) == n)
		{
			eraser = it;
			found = true;
		}
	}
	if (found)
		_instances.erase(eraser);
}

void Astra::Scene::addLight(Light* l)
{
	if (_lights.size() < MAX_LIGHTS)
		_lights.push_back(l);
	else
		Astra::Log("The maximum number of lights is " + std::to_string(MAX_LIGHTS) + "!", WARNING);
}

void Astra::Scene::removeLight(Light* l)
{
	auto eraser = _lights.begin();
	bool found = false;
	for (auto it = _lights.begin(); it != _lights.end() && !found; ++it)
	{
		if (*(*it) == *l)
		{
			eraser = it;
			found = true;
		}
	}
	if (found)
		_lights.erase(eraser);
}

void Astra::Scene::setCamera(CameraController* c)
{
	_camera = c;
}

void Astra::Scene::update(const CommandList& cmdList)
{
	// updating lights
	_lightsUniform = {};
	//_lightsUniform.nLights = _lights.size();
	for (int i = 0; i < _lights.size(); i++)
	{
		_lights[i]->update();
		_lightsUniform.lights[i] = _lights[i]->getLightSource();
	}
	updateLightsUBO(cmdList);

	// updating camera
	_camera->update();
	updateCameraUBO(cmdList);

	// updating instances
	for (auto& i : _instances)
	{
		i.update();
	}
}

void Astra::Scene::draw(RenderContext<PushConstantRaster>& renderContext)
{
	renderContext.pushConstant.nLights = _lights.size();
	for (auto& inst : _instances)
	{
		// skip invisibles
		if (inst.getVisible())
		{
			// get model (with buffers) and update transform matrix
			auto& model = _objModels[inst.getMeshIndex()];
			inst.updatePushConstantRaster(renderContext.pushConstant);

			// send pc to gpu
			renderContext.pushConstants();

			// draw call
			model.draw(renderContext.cmdList);
		}
	}
}

void Astra::SceneRT::draw(RenderContext<PushConstantRay>& renderContext)
{
	renderContext.pushConstant.nLights = _lights.size();
	renderContext.pushConstants();
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

std::vector<Astra::Mesh>& Astra::Scene::getModels()
{
	return _objModels;
}

std::vector<nvvk::Texture>& Astra::Scene::getTextures()
{
	return _textures;
}

nvvk::Buffer& Astra::Scene::getObjDescBuff()
{
	return _objDescBuffer;
}

nvvk::Buffer& Astra::Scene::getCameraUBO()
{
	return _cameraUBO;
}

nvvk::Buffer& Astra::Scene::getLightsUBO()
{
	return _lightsUBO;
}

void Astra::Scene::reset()
{
	// default raster scenes dont have to do anything by default
}

void Astra::Scene::updatePushConstantRaster(PushConstantRaster& pc)
{
	// TODO, futurure
}

void Astra::Scene::updatePushConstant(PushConstantRay& pc)
{
	// TODO, futurure
}

//===== DEFAULT RT SCENE =====

void Astra::SceneRT::init(nvvk::ResourceAllocator* alloc)
{
	Scene::init(alloc);
	_rtBuilder.setup(AstraDevice.getVkDevice(), alloc, Astra::Device::getInstance().getGraphicsQueueIndex());
	createBottomLevelAS();
	createTopLevelAS();
}

void Astra::SceneRT::update(const CommandList& cmdList)
{
	Astra::Scene::update(cmdList);
	std::vector<int> asupdates;
	for (int i = 0; i < _instances.size(); i++)
	{
		if (_instances[i].update())
		{
			asupdates.push_back(i);
		}
	}

	for (int i : asupdates)
	{
		updateTopLevelAS(i);
	}
}

void Astra::SceneRT::createBottomLevelAS()
{
	if (getTLAS() != VK_NULL_HANDLE)
	{
		_rtBuilder.destroy();
		_asInstances.clear();
	}
	std::vector<nvvk::RaytracingBuilderKHR::BlasInput> allBlas;
	allBlas.reserve(_objModels.size());

	for (const auto& obj : _objModels)
	{
		auto blas = AstraDevice.objectToVkGeometry(obj);

		allBlas.emplace_back(blas);
	}

	_rtBuilder.buildBlas(allBlas, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
}

void Astra::SceneRT::createTopLevelAS()
{
	if (getTLAS() != VK_NULL_HANDLE)
	{
		_rtBuilder.destroy();
		_asInstances.clear();
	}
	_asInstances.reserve(_instances.size());
	for (const Astra::MeshInstance& inst : _instances)
	{
		VkAccelerationStructureInstanceKHR rayInst{};
		rayInst.transform = nvvk::toTransformMatrixKHR(inst.getTransform());
		rayInst.instanceCustomIndex = inst.getMeshIndex(); // gl_InstanceCustomIndexEXT
		rayInst.accelerationStructureReference = _rtBuilder.getBlasDeviceAddress(inst.getMeshIndex());
		rayInst.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FRONT_COUNTERCLOCKWISE_BIT_KHR;
		rayInst.mask = inst.getVisible() ? 0xFF : 0x00;		// only be hit if raymask & instance.mask != 0
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
	rayInst.instanceCustomIndex = inst.getMeshIndex(); // gl_InstanceCustomIndexEXT
	rayInst.accelerationStructureReference = _rtBuilder.getBlasDeviceAddress(inst.getMeshIndex());
	rayInst.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FRONT_COUNTERCLOCKWISE_BIT_KHR;
	rayInst.mask = inst.getVisible() ? 0xFF : 0x00;		// only be hit if raymask & instance.mask != 0
	rayInst.instanceShaderBindingTableRecordOffset = 0; // the same hit group for all objects
	_asInstances[instance_id] = rayInst;

	_rtBuilder.buildTlas(_asInstances, VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR, true);
}

void Astra::SceneRT::rebuildAS()
{
	createBottomLevelAS();
	createTopLevelAS();
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

void Astra::SceneRT::reset()
{
	rebuildAS();
}
