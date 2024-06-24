#include <App.h>
#include <imgui.h>
#include <Device.h>
#include <nvvk/buffers_vk.hpp>
#include <Utils.h>
#include <glm/gtx/transform.hpp>

void Astra::App::updateUBO(CommandList& cmdList)
{
	GlobalUniforms hostUBO = _scenes[_currentScene]->getCamera()->getUpdatedGlobals();

	// UBO on the device, and what stages access it.
	VkBuffer deviceUBO = _globalsBuffer.buffer;
	auto uboUsageStages = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;

	// Ensure that the modified UBO is not visible to previous frames.
	VkBufferMemoryBarrier beforeBarrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
	beforeBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	beforeBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	beforeBarrier.buffer = deviceUBO;
	beforeBarrier.offset = 0;
	beforeBarrier.size = sizeof(hostUBO);
	cmdList.pipelineBarrier(uboUsageStages, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_DEVICE_GROUP_BIT, {}, { beforeBarrier }, {});

	// Schedule the host-to-device upload. (hostUBO is copied into the cmd
	// buffer so it is okay to deallocate when the function returns).
	cmdList.updateBuffer(_globalsBuffer, 0, sizeof(GlobalUniforms), &hostUBO);

	// Making sure the updated UBO will be visible.
	VkBufferMemoryBarrier afterBarrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
	afterBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	afterBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	afterBarrier.buffer = deviceUBO;
	afterBarrier.offset = 0;
	afterBarrier.size = sizeof(hostUBO);
	cmdList.pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, uboUsageStages, VK_DEPENDENCY_DEVICE_GROUP_BIT, {}, { afterBarrier }, {});
}

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
	VkDescriptorBufferInfo dbiUnif{ _globalsBuffer.buffer, 0, VK_WHOLE_SIZE };
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

void Astra::App::createUBO()
{
	// TODO change to device function with size param
	_globalsBuffer = _alloc.createBuffer(sizeof(GlobalUniforms), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
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



	createUBO();
	createDescriptorSetLayout();
	updateDescriptorSet();
	setupCallbacks(AstraDevice.getWindow());
	_gui->init(AstraDevice.getWindow(), _renderer);
}

void Astra::App::addScene(Scene* s)
{
	_scenes.push_back(s);
}

void Astra::App::run()
{
	_scenes[_currentScene]->update();
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

	_alloc.destroy(_globalsBuffer);

	for (auto s : _scenes)
		s->destroy();

	destroyPipelines();
}

nvvk::Buffer& Astra::App::getCameraUBO()
{
	return _globalsBuffer;
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
