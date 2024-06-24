#include <Scene.h>
#include <host_device.h>
#include <Device.h>
#include <nvvk/buffers_vk.hpp>
#include <Utils.h>

void Astra::Scene::createObjDescBuffer()
{
	if (_objDescBuffer.buffer != VK_NULL_HANDLE) {
		_alloc->destroy(_objDescBuffer);
	}

	nvvk::CommandPool cmdGen(AstraDevice.getVkDevice(), AstraDevice.getGraphicsQueueIndex());

	auto cmdBuf = cmdGen.createCommandBuffer();
	std::vector<ObjDesc> objDescs;
	for (auto& mesh : _objModels) {
		objDescs.push_back(mesh.descriptor);
	}
	if (!objDescs.empty())
		_objDescBuffer = _alloc->createBuffer(cmdBuf, objDescs, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

	cmdGen.submitAndWait(cmdBuf);
	_alloc->finalizeAndReleaseStaging();
}

void Astra::Scene::loadModel(const std::string& filename, const glm::mat4& transform)
{
	// we cant load models until we have access to the resource allocator
	// if we have it, just create it
	// if we dont, postpone the operation to the init stage
	if (_alloc) {
		// find the offset for this model
		auto txtOffset = static_cast<uint32_t>(getTextures().size());
		// allocating cmdbuffers
		nvvk::CommandPool  cmdBufGet(AstraDevice.getVkDevice(), AstraDevice.getGraphicsQueueIndex());
		VkCommandBuffer    cmdBuf = cmdBufGet.createCommandBuffer();
		Astra::CommandList cmdList(cmdBuf);

		Astra::Mesh mesh;
		mesh.loadFromFile(filename);
		mesh.meshId = getModels().size();

		// color space to linear
		for (auto& m : mesh.materials) {
			m.ambient = glm::pow(m.ambient, glm::vec3(2.2f));
			m.diffuse = glm::pow(m.diffuse, glm::vec3(2.2f));
			m.specular = glm::pow(m.specular, glm::vec3(2.2f));
		}

		// creates the buffers and descriptors neeeded
		mesh.create(cmdList, _alloc, txtOffset);

		// Creates all textures found 
		AstraDevice.createTextureImages(cmdBuf, mesh.textures, getTextures(), *_alloc);
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
	else {
		_lazymodels.push_back(std::make_pair(filename, transform));
	}
}

void Astra::Scene::init(nvvk::ResourceAllocator* alloc)
{
	_alloc = (nvvk::ResourceAllocatorDma*)alloc;
	for (auto& p : _lazymodels) {
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

void Astra::Scene::addModel(const Astra::Mesh& model)
{
	_objModels.push_back(model);
}

void Astra::Scene::addInstance(const MeshInstance& instance)
{
	_instances.push_back(instance);
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

void Astra::Scene::updatePushConstantRaster(PushConstantRaster& pc)
{
	// TODO, futurure
}

void Astra::Scene::updatePushConstant(PushConstantRay& pc)
{
	// TODO, futurure
}

