#include <App.h>
#include <imgui.h>
#include <Device.h>
#include <nvvk/buffers_vk.hpp>
#include <Utils.h>
#include <glm/gtx/transform.hpp>

void Astra::App::destroyPipelines()
{
	AstraDevice.waitIdle();
	for (auto p : _pipelines)
	{
		p->destroy(&_alloc);
		delete p;
	}
}

void Astra::App::createDescriptorSetLayout()
{
	int nbTxt = _scenes[_currentScene]->getTextures().size();

	// Camera matrices
	_descSetLayoutBind.addBinding(SceneBindings::eCamera, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
		VK_SHADER_STAGE_VERTEX_BIT | (AstraDevice.getRtEnabled() ? (VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR) : 0));
	// Lights
	_descSetLayoutBind.addBinding(SceneBindings::eLights, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
		VK_SHADER_STAGE_FRAGMENT_BIT | (AstraDevice.getRtEnabled() ? (VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR) : 0));
	// Obj descriptions
	_descSetLayoutBind.addBinding(SceneBindings::eObjDescs, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | (AstraDevice.getRtEnabled() ? (VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR) : 0));
	// Textures
	_descSetLayoutBind.addBinding(SceneBindings::eTextures, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nbTxt,
		VK_SHADER_STAGE_FRAGMENT_BIT | (AstraDevice.getRtEnabled() ? (VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR) : 0));

	_descSetLayout = _descSetLayoutBind.createLayout(AstraDevice.getVkDevice());
	_descPool = _descSetLayoutBind.createPool(AstraDevice.getVkDevice(), 1);
	_descSet = nvvk::allocateDescriptorSet(AstraDevice.getVkDevice(), _descPool, _descSetLayout);
}

void Astra::App::updateDescriptorSet()
{
	std::vector<VkWriteDescriptorSet> writes;

	// Camera matrices and scene description
	VkDescriptorBufferInfo dbiCamUnif{ _scenes[_currentScene]->getCameraUBO().buffer, 0, VK_WHOLE_SIZE };
	writes.emplace_back(_descSetLayoutBind.makeWrite(_descSet, SceneBindings::eCamera, &dbiCamUnif));

	VkDescriptorBufferInfo dbiLightUnif{ _scenes[_currentScene]->getLightsUBO().buffer, 0, VK_WHOLE_SIZE };
	writes.emplace_back(_descSetLayoutBind.makeWrite(_descSet, SceneBindings::eLights, &dbiLightUnif));

	VkDescriptorBufferInfo dbiSceneDesc{ _scenes[_currentScene]->getObjDescBuff().buffer, 0, VK_WHOLE_SIZE };
	writes.emplace_back(_descSetLayoutBind.makeWrite(_descSet, SceneBindings::eObjDescs, &dbiSceneDesc));

	// All texture samplers
	std::vector<VkDescriptorImageInfo> diit;
	// for (int i = 0; i < _scenes.size(); i++) {

	for (auto& texture : _scenes[_currentScene]->getTextures())
	{
		diit.emplace_back(texture.descriptor);
	}
	//}
	writes.emplace_back(_descSetLayoutBind.makeWriteArray(_descSet, SceneBindings::eTextures, diit.data()));

	// Writing the information
	vkUpdateDescriptorSets(AstraDevice.getVkDevice(), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}

void Astra::App::resetScene(bool recreatePipelines)
{
	_scenes[_currentScene]->reset();
	_descSetLayoutBind.clear();
	createDescriptorSetLayout();
	updateDescriptorSet();

	if (recreatePipelines)
	{
		destroyPipelines();
		createPipelines();
	}

}

void Astra::App::onResize(int w, int h)
{
	if (w == 0 || h == 0)
		return;

	if (_gui)
	{
		auto& imgui_io = ImGui::GetIO();
		imgui_io.DisplaySize = ImVec2(static_cast<float>(w), static_cast<float>(h));
	}

	// wait until finishing tasks
	AstraDevice.waitIdle();
	AstraDevice.queueWaitIdle();

	// resize renderer
	_renderer->resize(w, h, _alloc);

	updateDescriptorSet();
	_scenes[_currentScene]->getCamera()->setWindowSize(w, h);
}

void Astra::App::init(const std::vector<Scene*>& scenes, Renderer* renderer, GuiController* gui)
{
	_status = Running;
	_window = AstraDevice.getWindow();

	Input.init(_window, this);

	_alloc.init(AstraDevice.getVkDevice(), AstraDevice.getPhysicalDevice());
	for (auto s : scenes)
	{
		s->init(&_alloc);
	}
	_scenes = scenes;
	_renderer = renderer;
	_gui = gui;

	// renderer init
	_renderer->init(this, _alloc);

	createDescriptorSetLayout();
	updateDescriptorSet();
	if (_gui != nullptr)
		_gui->init(_window, _renderer);
}

void Astra::App::addScene(Scene* s)
{
	_scenes.push_back(s);
}

Astra::App::~App()
{
	_alloc.deinit();
}

void Astra::App::destroy()
{
	if (_status == Running)
	{ // if the app failed to init, we dont destroy it

		_status = Destroyed;

		const auto& device = AstraDevice.getVkDevice();

		AstraDevice.waitIdle();

		_renderer->destroy(&_alloc);

		if (_gui != nullptr)
			_gui->destroy();

		for (auto s : _scenes)
			s->destroy();

		vkDestroyDescriptorSetLayout(AstraDevice.getVkDevice(), _descSetLayout, nullptr);

		destroyPipelines();
	}
}

bool Astra::App::isMinimized() const
{
	int w, h;
	glfwGetWindowSize(AstraDevice.getWindow(), &w, &h);

	return w == 0 || h == 0;
}

int& Astra::App::getCurrentSceneIndexRef()
{
	return _currentScene;
}

int Astra::App::getCurrentSceneIndex() const
{
	return _currentScene;
}

void Astra::App::setCurrentSceneIndex(int i)
{
	if (i >= 0 && i < _scenes.size())
	{
		_currentScene = i;
	}
	else
	{
		Astra::Log("Invalid scene index", WARNING);
	}
}

Astra::Scene* Astra::App::getCurrentScene()
{
	return _scenes[_currentScene];
}

Astra::Renderer* Astra::App::getRenderer()
{
	return _renderer;
}

Astra::AppStatus Astra::App::getStatus() const
{
	return _status;
}