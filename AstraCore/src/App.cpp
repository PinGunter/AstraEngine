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
	// TODO rework into different descriptor for texturesss
	int nbTxt = _scenes[_currentScene]->getTextures().size();

	// Camera matrices
	_descSetLayoutBind.addBinding(SceneBindings::eGlobals, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
	// Obj descriptions
	_descSetLayoutBind.addBinding(SceneBindings::eObjDescs, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
	// Textures
	_descSetLayoutBind.addBinding(SceneBindings::eTextures, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nbTxt,
		VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);

	_descSetLayout = _descSetLayoutBind.createLayout(AstraDevice.getVkDevice());
	_descPool = _descSetLayoutBind.createPool(AstraDevice.getVkDevice(), 1);
	_descSet = nvvk::allocateDescriptorSet(AstraDevice.getVkDevice(), _descPool, _descSetLayout);
}

void Astra::App::updateDescriptorSet()
{
	std::vector<VkWriteDescriptorSet> writes;

	// Camera matrices and scene description
	VkDescriptorBufferInfo dbiUnif{ _scenes[_currentScene]->getCameraUBO().buffer, 0, VK_WHOLE_SIZE };
	writes.emplace_back(_descSetLayoutBind.makeWrite(_descSet, SceneBindings::eGlobals, &dbiUnif));

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

	// request swapchain image
	_renderer->requestSwapchainImage(w, h);

	_scenes[_currentScene]->getCamera()->setWindowSize(w, h);

	_renderer->createOffscreenRender(_alloc);

	_renderer->updatePostDescriptorSet();
	updateDescriptorSet();

	_renderer->createDepthBuffer();
	_renderer->createFrameBuffers();
}

void Astra::App::init(const std::vector<Scene*>& scenes, Renderer* renderer, GuiController* gui)
{
	_status = Running;
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
	setupCallbacks(AstraDevice.getWindow());
	_gui->init(AstraDevice.getWindow(), _renderer);
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
	_status = Destroyed;

	const auto& device = AstraDevice.getVkDevice();

	AstraDevice.waitIdle();

	_renderer->destroy(&_alloc);

	_gui->destroy();


	for (auto s : _scenes)
		s->destroy();

	destroyPipelines();
}

void Astra::App::setupCallbacks(GLFWwindow* window)
{
	_window = window;
	glfwSetWindowUserPointer(window, this);
	glfwSetKeyCallback(window, &cb_keyboard);
	glfwSetCharCallback(window, &cb_keyboardChar);
	glfwSetCursorPosCallback(window, &cb_mouseMotion);
	glfwSetMouseButtonCallback(window, &cb_mouseButton);
	glfwSetScrollCallback(window, &cb_mouseWheel);
	glfwSetFramebufferSizeCallback(window, &cb_resize);
	glfwSetDropCallback(window, &cb_fileDrop);
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

void Astra::App::cb_resize(GLFWwindow* window, int w, int h)
{
	auto app = static_cast<Astra::App*>(glfwGetWindowUserPointer(window));
	app->onResize(w, h);
}

void Astra::App::cb_keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	auto app = static_cast<Astra::App*>(glfwGetWindowUserPointer(window));
	app->onKeyboard(key, scancode, action, mods);
}

void Astra::App::cb_keyboardChar(GLFWwindow* window, unsigned int key)
{
	auto app = static_cast<Astra::App*>(glfwGetWindowUserPointer(window));
	app->onKeyboardChar(key);
}

void Astra::App::cb_mouseMotion(GLFWwindow* window, double x, double y)
{
	auto app = static_cast<Astra::App*>(glfwGetWindowUserPointer(window));
	app->onMouseMotion(x, y);
}

void Astra::App::cb_mouseButton(GLFWwindow* window, int button, int action, int mods)
{
	auto app = static_cast<Astra::App*>(glfwGetWindowUserPointer(window));
	app->onMouseButton(button, action, mods);
}

void Astra::App::cb_mouseWheel(GLFWwindow* window, double x, double y)
{
	auto app = static_cast<Astra::App*>(glfwGetWindowUserPointer(window));
	app->onMouseWheel(x, y);
}

void Astra::App::cb_fileDrop(GLFWwindow* window, int count, const char** paths)
{
	auto app = static_cast<Astra::App*>(glfwGetWindowUserPointer(window));
	app->onFileDrop(count, paths);
}
