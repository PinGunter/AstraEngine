#include <App.h>
#include <imgui.h>
#include <Device.h>
#include <nvvk/buffers_vk.hpp>

#include <obj_loader.h>
#include <nvh/fileoperations.hpp>
#include <nvpsystem.hpp>
#include <Utils.h>
#include <glm/gtx/transform.hpp>


void Astra::App::updateUBO(CommandList& cmdList)
{
	GlobalUniforms hostUBO = _scenes[_currentScene]->getCamera()->getUpdatedGlobals();

	// UBO on the device, and what stages access it.
	VkBuffer deviceUBO = _globalsBuffer.buffer;
	auto     uboUsageStages = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;

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
	for (auto p : _pipelines) {
		p->destroy(&_alloc);
		delete p;
	}
}

void Astra::App::createUBO()
{
	//TODO change to device function with size param
	_globalsBuffer = _alloc.createBuffer(sizeof(GlobalUniforms), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

void Astra::App::init(const std::vector<Scene*>& scenes, Renderer* renderer, GuiController* gui)
{
	_status = Running;
	_alloc.init(AstraDevice.getVkDevice(), AstraDevice.getPhysicalDevice());
	for (auto s : scenes) {
		s->init(&_alloc);
	}
	_scenes = scenes;
	_renderer = renderer;
	_renderer->linkApp(this);
	_gui = gui;
	setupCallbacks(AstraDevice.getWindow());
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
	if (i >= 0 && i < _scenes.size()) {
		_currentScene = i;
	}
	else {
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
	auto app = static_cast<Astra::App*> (glfwGetWindowUserPointer(window));
	app->onResize(w, h);
}

void Astra::App::cb_keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	auto app = static_cast<Astra::App*> (glfwGetWindowUserPointer(window));
	app->onKeyboard(key, scancode, action, mods);
}

void Astra::App::cb_keyboardChar(GLFWwindow* window, unsigned int key)
{
	auto app = static_cast<Astra::App*> (glfwGetWindowUserPointer(window));
	app->onKeyboardChar(key);
}

void Astra::App::cb_mouseMotion(GLFWwindow* window, double x, double y)
{
	auto app = static_cast<Astra::App*> (glfwGetWindowUserPointer(window));
	app->onMouseMotion(x, y);
}

void Astra::App::cb_mouseButton(GLFWwindow* window, int button, int action, int mods)
{
	auto app = static_cast<Astra::App*> (glfwGetWindowUserPointer(window));
	app->onMouseButton(button, action, mods);
}

void Astra::App::cb_mouseWheel(GLFWwindow* window, double x, double y)
{
	auto app = static_cast<Astra::App*> (glfwGetWindowUserPointer(window));
	app->onMouseWheel(x, y);
}

void Astra::App::cb_fileDrop(GLFWwindow* window, int count, const char** paths)
{
	auto app = static_cast<Astra::App*> (glfwGetWindowUserPointer(window));
	app->onFileDrop(count, paths);
}


void Astra::DefaultApp::init(const std::vector<Scene*>& scenes, Renderer* renderer, GuiController* gui)
{
	// init base app, copies scenes renderer and gui
	// also inits the scenes and sets callbacks
	Astra::App::init(scenes, renderer, gui);

	// raytracing init
	VkPhysicalDeviceProperties2 prop2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
	prop2.pNext = &_rtProperties;
	vkGetPhysicalDeviceProperties2(AstraDevice.getPhysicalDevice(), &prop2);

	// renderer init
	auto size = AstraDevice.getWindowSize();
	_renderer->createSwapchain(AstraDevice.getSurface(), size[0], size[1]);
	_renderer->createDepthBuffer();
	_renderer->createRenderPass();
	_renderer->createFrameBuffers();
	_renderer->createOffscreenRender(_alloc);
	_renderer->createPostDescriptorSet();
	_renderer->createPostPipeline();
	_renderer->updatePostDescriptorSet();

	// gui init
	_gui = gui;
	_gui->init(AstraDevice.getWindow(), _renderer);

	// Scene -> GPU information || Uniforms
	// camera uniforms
	createUBO();

	// aceleration structures
	for (Astra::Scene* s : _scenes) {
		if (s->isRt()) {
			((Astra::DefaultSceneRT*)s)->createBottomLevelAS();
			((Astra::DefaultSceneRT*)s)->createTopLevelAS();
		}
	}

	// descriptor sets
	createDescriptorSetLayout();
	updateDescriptorSet();
	createRtDescriptorSet();
	// pipelines
	createPipelines();
}

void Astra::DefaultApp::run()
{
	while (!glfwWindowShouldClose(_window)) {
		App::run(); // update the scene

		glfwPollEvents();
		if (isMinimized()) {
			continue;
		}

		if (_needsReset) {
			resetScene(_fullReset);
		}

		_rendering = true;
		auto cmdList = _renderer->beginFrame();
		updateUBO(cmdList);

		// offscren render

		if (_selectedPipeline == 0) {
			_renderer->render(cmdList, _scenes[_currentScene], _rtPipeline, { _rtDescSet, _descSet }, _gui);
		}
		else if (_selectedPipeline == 1) {
			_renderer->render(cmdList, _scenes[_currentScene], _rasterPipeline, { _descSet }, _gui);
		}
		else if (_selectedPipeline == 2) {
			_renderer->render(cmdList, _scenes[_currentScene], _wireframePipeline, { _descSet }, _gui);
		}


		_renderer->endFrame(cmdList);
		_rendering = false;

		AstraDevice.waitIdle();
		for (auto& model_pair : _newModels) {
			addModelToScene(model_pair.first, model_pair.second);
		}
		_newModels.clear();

		for (auto& inst : _newInstances) {
			addInstanceToScene(inst);
		}
		_newInstances.clear();

	}
	destroy();
}

void Astra::DefaultApp::destroy()
{
	App::destroy();
	vkDestroyDescriptorSetLayout(AstraDevice.getVkDevice(), _descSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(AstraDevice.getVkDevice(), _rtDescSetLayout, nullptr);
	vkDestroyDescriptorPool(AstraDevice.getVkDevice(), _rtDescPool, nullptr);
}

void Astra::DefaultApp::createPipelines()
{
	// raytracing pipeline
	_rtPipeline = new RayTracingPipeline();
	((RayTracingPipeline*)_rtPipeline)->createPipeline(AstraDevice.getVkDevice(), { _rtDescSetLayout, _descSetLayout }, _alloc, _rtProperties);

	// basic raster
	_rasterPipeline = new OffscreenRaster();
	((OffscreenRaster*)_rasterPipeline)->createPipeline(AstraDevice.getVkDevice(), { _descSetLayout }, _renderer->getOffscreenRenderPass());

	// wireframe
	_wireframePipeline = new WireframePipeline();
	((WireframePipeline*)_wireframePipeline)->createPipeline(AstraDevice.getVkDevice(), { _descSetLayout }, _renderer->getOffscreenRenderPass());

	_pipelines = { _rtPipeline, _rasterPipeline, _wireframePipeline };
}

void Astra::DefaultApp::createDescriptorSetLayout()
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

// Default App

void Astra::DefaultApp::updateDescriptorSet()
{
	std::vector<VkWriteDescriptorSet> writes;

	// Camera matrices and scene description
	VkDescriptorBufferInfo dbiUnif{ _globalsBuffer.buffer, 0, VK_WHOLE_SIZE };
	writes.emplace_back(_descSetLayoutBind.makeWrite(_descSet, SceneBindings::eGlobals, &dbiUnif));

	VkDescriptorBufferInfo dbiSceneDesc{ _scenes[_currentScene]->getObjDescBuff().buffer, 0, VK_WHOLE_SIZE };
	writes.emplace_back(_descSetLayoutBind.makeWrite(_descSet, SceneBindings::eObjDescs, &dbiSceneDesc));

	// All texture samplers
	std::vector<VkDescriptorImageInfo> diit;
	//for (int i = 0; i < _scenes.size(); i++) {

	for (auto& texture : _scenes[_currentScene]->getTextures())
	{
		diit.emplace_back(texture.descriptor);
	}
	//}
	writes.emplace_back(_descSetLayoutBind.makeWriteArray(_descSet, SceneBindings::eTextures, diit.data()));

	// Writing the information
	vkUpdateDescriptorSets(AstraDevice.getVkDevice(), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}

void Astra::DefaultApp::createRtDescriptorSet()
{
	const auto& device = AstraDevice.getVkDevice();
	_rtDescSetLayoutBind.addBinding(RtxBindings::eTlas, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
	_rtDescSetLayoutBind.addBinding(RtxBindings::eOutImage, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR);

	_rtDescPool = _rtDescSetLayoutBind.createPool(device);
	_rtDescSetLayout = _rtDescSetLayoutBind.createLayout(device);

	VkDescriptorSetAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	allocateInfo.descriptorPool = _rtDescPool;
	allocateInfo.descriptorSetCount = 1;
	allocateInfo.pSetLayouts = &_rtDescSetLayout;
	vkAllocateDescriptorSets(device, &allocateInfo, &_rtDescSet);

	VkAccelerationStructureKHR tlas = ((DefaultSceneRT*)_scenes[_currentScene])->getTLAS();
	VkWriteDescriptorSetAccelerationStructureKHR descASInfo{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR };
	descASInfo.accelerationStructureCount = 1;
	descASInfo.pAccelerationStructures = &tlas;
	VkDescriptorImageInfo imageInfo{ {},_renderer->getOffscreenColor().descriptor.imageView, VK_IMAGE_LAYOUT_GENERAL };

	std::vector<VkWriteDescriptorSet> writes;
	writes.emplace_back(_rtDescSetLayoutBind.makeWrite(_rtDescSet, RtxBindings::eTlas, &descASInfo));
	writes.emplace_back(_rtDescSetLayoutBind.makeWrite(_rtDescSet, RtxBindings::eOutImage, &imageInfo));
	vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}

void Astra::DefaultApp::updateRtDescriptorSet()
{
	VkDescriptorImageInfo imageInfo{ {}, _renderer->getOffscreenColor().descriptor.imageView, VK_IMAGE_LAYOUT_GENERAL };
	VkWriteDescriptorSet wds = _rtDescSetLayoutBind.makeWrite(_rtDescSet, RtxBindings::eOutImage, &imageInfo);
	vkUpdateDescriptorSets(AstraDevice.getVkDevice(), 1, &wds, 0, nullptr);
}


void Astra::DefaultApp::onResize(int w, int h)
{
	if (w == 0 || h == 0)
		return;

	if (_gui) {
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
	updateRtDescriptorSet();
	updateDescriptorSet();

	_renderer->createDepthBuffer();
	_renderer->createFrameBuffers();

}

void Astra::DefaultApp::onMouseMotion(int x, int y)
{
	auto s = _scenes[_currentScene];
	int delta[2] = { x - _lastMousePos[0], y - _lastMousePos[1] };
	_lastMousePos[0] = x;
	_lastMousePos[1] = y;
	s->getCamera()->handleMouseInput(_mouseButtons, delta, 0, _inputMods);
}

void Astra::DefaultApp::onMouseButton(int button, int action, int mods)
{
	_mouseButtons[button] = action == GLFW_PRESS;
	auto s = _scenes[_currentScene];
	int _[2];
	_[0] = _[1] = 0;
	_inputMods = mods;
	s->getCamera()->handleMouseInput(_mouseButtons, _, 0, _inputMods);
}

void Astra::DefaultApp::onMouseWheel(int x, int y)
{
	auto s = _scenes[_currentScene];
	int _[2];
	_[0] = _[1] = 0;
	s->getCamera()->handleMouseInput(_mouseButtons, _, 10 * y, _inputMods);
}

void Astra::DefaultApp::onKeyboard(int key, int scancode, int action, int mods)
{
	// pass;
}

void Astra::DefaultApp::onFileDrop(int count, const char** paths)
{
	if (count == 1) {
		std::string string_path(paths[0]);
		// check for obj object
		if (string_path.substr(string_path.size() - 4, string_path.size()) == ".obj") {
			addModelToScene(string_path);
		}
	}
	else {
		Astra::Log("Only one model at a time please!", INFO); // right now only one.
	}
}

void Astra::DefaultApp::setCurrentSceneIndex(int i)
{
	int nbTxt = _scenes[_currentScene]->getTextures().size();
	Astra::App::setCurrentSceneIndex(i);
	if (_status == Running) {
		scheduleReset(nbTxt != _scenes[_currentScene]->getTextures().size());
	}
}

void Astra::DefaultApp::resetScene(bool recreatePipelines)
{
	((Astra::DefaultSceneRT*)_scenes[_currentScene])->createBottomLevelAS();
	((Astra::DefaultSceneRT*)_scenes[_currentScene])->createTopLevelAS();
	_descSetLayoutBind.clear();
	_rtDescSetLayoutBind.clear();
	createDescriptorSetLayout();
	updateDescriptorSet();
	createRtDescriptorSet();

	if (recreatePipelines) {
		destroyPipelines();
		createPipelines();
	}
	_needsReset = false;
}

void Astra::DefaultApp::scheduleReset(bool recreatePipelines)
{
	_needsReset = true;
	_fullReset = recreatePipelines;
}

void Astra::DefaultApp::addModelToScene(const std::string& filepath, const glm::mat4& transform)
{
	if (_rendering) {
		_newModels.push_back(std::make_pair(filepath, transform));
	}
	else {
		int currentTxtSize = _scenes[_currentScene]->getTextures().size();
		_scenes[_currentScene]->loadModel(filepath, transform);
		resetScene(_scenes[_currentScene]->getTextures().size() > currentTxtSize);
	}
}

void Astra::DefaultApp::addInstanceToScene(const Astra::MeshInstance& instance)
{
	if (_rendering) {
		_newInstances.push_back(instance);
	}
	else {
		_scenes[_currentScene]->addInstance(instance);
		resetScene();
	}
}

int& Astra::DefaultApp::getSelectedPipelineRef()
{
	return _selectedPipeline;
}
