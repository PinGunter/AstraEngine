#include <App.h>
#include <imgui.h>
#include <Device.h>
#include <nvvk/buffers_vk.hpp>

#include <obj_loader.h>
#include <nvh/fileoperations.hpp>
#include <nvpsystem.hpp>
#include <Utils.h>


void Astra::App::updateUBO(const VkCommandBuffer& cmdBuf)
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
	vkCmdPipelineBarrier(cmdBuf, uboUsageStages, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_DEVICE_GROUP_BIT, 0,
		nullptr, 1, &beforeBarrier, 0, nullptr);


	// Schedule the host-to-device upload. (hostUBO is copied into the cmd
	// buffer so it is okay to deallocate when the function returns).
	vkCmdUpdateBuffer(cmdBuf, _globalsBuffer.buffer, 0, sizeof(GlobalUniforms), &hostUBO);

	// Making sure the updated UBO will be visible.
	VkBufferMemoryBarrier afterBarrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
	afterBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	afterBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	afterBarrier.buffer = deviceUBO;
	afterBarrier.offset = 0;
	afterBarrier.size = sizeof(hostUBO);
	vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TRANSFER_BIT, uboUsageStages, VK_DEPENDENCY_DEVICE_GROUP_BIT, 0,
		nullptr, 1, &afterBarrier, 0, nullptr);
}

void Astra::App::createUBO()
{
	_globalsBuffer = Astra::Device::getInstance().getResAlloc().createBuffer(sizeof(GlobalUniforms), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

void Astra::App::createObjDescBuffer()
{
	nvvk::CommandPool cmdGen(Astra::Device::getInstance().getVkDevice(), Astra::Device::getInstance().getGraphicsQueueIndex());

	auto cmdBuf = cmdGen.createCommandBuffer();
	for (auto s : _scenes) {
		if (!s->getObjDesc().empty())
			s->getObjDescBuff() = Astra::Device::getInstance().getResAlloc().createBuffer(cmdBuf, s->getObjDesc(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
	}
	cmdGen.submitAndWait(cmdBuf);
	Astra::Device::getInstance().getResAlloc().finalizeAndReleaseStaging();
}



void Astra::App::init(const std::vector<Scene*>& scenes, Renderer* renderer, GuiController* gui)
{
	_scenes = scenes;
	_renderer = renderer;
	_renderer->linkApp(this);
	_gui = gui;
}

void Astra::App::addScene(Scene* s)
{
	_scenes.push_back(s);
}

void Astra::App::destroy()
{
	const auto& device = Astra::Device::getInstance().getVkDevice();
	auto& alloc = Astra::Device::getInstance().getResAlloc();
	
	vkDeviceWaitIdle(device);

	_renderer->destroy();
	for (auto s : _scenes)
		s->destroy();

	alloc.destroy(_globalsBuffer);
}

nvvk::Buffer & Astra::App::getCameraUBO()
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
	glfwGetWindowSize(Astra::Device::getInstance().getWindow(), &w, &h);
	
	return w == 0 || h == 0;
}

void Astra::App::loadModel(const std::string& filename, const glm::mat4& transform)
{
	ObjLoader loader;
	loader.loadModel(filename);

	// Converting from Srgb to linear
	for (auto& m : loader.m_materials)
	{
		m.ambient = glm::pow(m.ambient, glm::vec3(2.2f));
		m.diffuse = glm::pow(m.diffuse, glm::vec3(2.2f));
		m.specular = glm::pow(m.specular, glm::vec3(2.2f));
	}

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

	_scenes[_currentScene]->addModel(model);

	// Create the buffers on Device and copy vertices, indices and materials
	nvvk::CommandPool  cmdBufGet(Astra::Device::getInstance().getVkDevice(), Astra::Device::getInstance().getGraphicsQueueIndex());
	VkCommandBuffer    cmdBuf = cmdBufGet.createCommandBuffer();
	VkBufferUsageFlags flag = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	VkBufferUsageFlags rayTracingFlags = flag | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	model.vertexBuffer = Astra::Device::getInstance().getResAlloc().createBuffer(cmdBuf, loader.m_vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | rayTracingFlags);
	model.indexBuffer = Astra::Device::getInstance().getResAlloc().createBuffer(cmdBuf, loader.m_indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | rayTracingFlags);
	model.matColorBuffer = Astra::Device::getInstance().getResAlloc().createBuffer(cmdBuf, loader.m_materials, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | rayTracingFlags);
	model.matIndexBuffer = Astra::Device::getInstance().getResAlloc().createBuffer(cmdBuf, loader.m_matIndx, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | rayTracingFlags);
	
	// Creates all textures found and find the offset for this model
	auto txtOffset = static_cast<uint32_t>(_scenes[_currentScene]->getTextures().size());
	Astra::Device::getInstance().createTextureImages(cmdBuf, loader.m_textures, _scenes[_currentScene]->getTextures());
	cmdBufGet.submitAndWait(cmdBuf);
	Astra::Device::getInstance().getResAlloc().finalizeAndReleaseStaging();

	// Creating information for device access
	ObjDesc desc;
	desc.txtOffset = txtOffset;
	desc.vertexAddress = nvvk::getBufferDeviceAddress(Astra::Device::getInstance().getVkDevice(), model.vertexBuffer.buffer);
	desc.indexAddress = nvvk::getBufferDeviceAddress(Astra::Device::getInstance().getVkDevice(), model.indexBuffer.buffer);
	desc.materialAddress = nvvk::getBufferDeviceAddress(Astra::Device::getInstance().getVkDevice(), model.matColorBuffer.buffer);
	desc.materialIndexAddress = nvvk::getBufferDeviceAddress(Astra::Device::getInstance().getVkDevice(), model.matIndexBuffer.buffer);
	
	
	// Keeping the obj host model and device description
	_scenes[_currentScene]->addModel(model);
	_scenes[_currentScene]->addObjDesc(desc);
	
	// Keeping transformation matrix of the instance
	Astra::MeshInstance instance(static_cast<uint32_t>(_scenes[_currentScene]->getModels().size()), transform, filename.substr(filename.size() - 10, filename.size()));
	_scenes[_currentScene]->addInstance(instance);

}

int& Astra::App::getCurrentSceneIndexRef()
{
	return _currentScene;
}

int Astra::App::getCurrenSceneIndex() const
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
	Astra::App::init(scenes, renderer, gui);

	// search paths for finding files
	std::vector<std::string> defaultSearchPaths = {
		NVPSystem::exePath() + PROJECT_RELDIRECTORY,
		NVPSystem::exePath() + PROJECT_RELDIRECTORY "..",
		std::string(PROJECT_NAME),
	};

	auto size = Astra::Device::getInstance().getWindowSize();
	_renderer->createSwapchain(Astra::Device::getInstance().getSurface(), size[0], size[1]);
	_renderer->createDepthBuffer();
	_renderer->createRenderPass();
	_renderer->createFrameBuffers();

	// gui init

	// loading models
	loadModel(nvh::findFile("media/scenes/mono.obj", defaultSearchPaths, true));

	_renderer->createOffscreenRender();
	createDescriptorSetLayout();
	((OffscreenRaster*)_rasterPipeline)->createPipeline(Astra::Device::getInstance().getVkDevice(), { _descSetLayout }, _renderer->getOffscreenRenderPass());

	createUBO();
	createObjDescBuffer();
	updateDescriptorSet();

	for (Astra::Scene* s : _scenes) {
		if (s->isRt()) {
			((Astra::SceneRT*)s)->createBottomLevelAS();
			((Astra::SceneRT*)s)->createTopLevelAS();
		}
	}

	createRtDescriptorSet();
	((RayTracingPipeline*)_rtPipeline)->createPipeline(Astra::Device::getInstance().getVkDevice(), { _rtDescSetLayout, _descSetLayout });

	_renderer->createPostDescriptorSet();
	_renderer->createPostPipeline();
	_renderer->updatePostDescriptorSet();
}

void Astra::DefaultApp::run()
{
	while (!glfwWindowShouldClose(_window)) {
		glfwPollEvents();
		if (isMinimized()) {
			continue;
		}

		// imgui new frame, imguizmo begin frame
	
		auto cmdBuf = _renderer->beginFrame();

		updateUBO(cmdBuf);

		// offscren render
		if (_useRT) {
			_renderer->render(cmdBuf, _scenes[_currentScene], _rtPipeline, { _rtDescSet, _descSet});
		}
		else {
			_renderer->render(cmdBuf, _scenes[_currentScene], _rasterPipeline, { _descSet });
		}

		// post render: ui and texture
		_renderer->beginPost();
		_renderer->renderPost(cmdBuf);
		// imgui::render()
		// imgui::renderdrawdata
		_renderer->endPost(cmdBuf);

		_renderer->endFrame();
		// render ui
	}
	destroy();
}

void Astra::DefaultApp::createDescriptorSetLayout()
{
	auto nbTxt = static_cast<uint32_t>(_scenes[0]->getTextures().size());

	// Camera matrices
	_descSetLayoutBind.addBinding(SceneBindings::eGlobals, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
	// Obj descriptions
	_descSetLayoutBind.addBinding(SceneBindings::eObjDescs, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
	// Textures
	_descSetLayoutBind.addBinding(SceneBindings::eTextures, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nbTxt,
		VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);


	_descSetLayout = _descSetLayoutBind.createLayout(Astra::Device::getInstance().getVkDevice());
	_descPool = _descSetLayoutBind.createPool(Astra::Device::getInstance().getVkDevice(), 1);
	_descSet = nvvk::allocateDescriptorSet(Astra::Device::getInstance().getVkDevice(), _descPool, _descSetLayout);
}

// Default App

void Astra::DefaultApp::updateDescriptorSet()
{
	std::vector<VkWriteDescriptorSet> writes;

	// Camera matrices and scene description
	VkDescriptorBufferInfo dbiUnif{ _globalsBuffer.buffer, 0, VK_WHOLE_SIZE };
	writes.emplace_back(_descSetLayoutBind.makeWrite(_descSet, SceneBindings::eGlobals, &dbiUnif));

	VkDescriptorBufferInfo dbiSceneDesc{ _scenes[0]->getObjDescBuff().buffer, 0, VK_WHOLE_SIZE};
	writes.emplace_back(_descSetLayoutBind.makeWrite(_descSet, SceneBindings::eObjDescs, &dbiSceneDesc));

	// All texture samplers
	std::vector<VkDescriptorImageInfo> diit;
	for (auto& texture : _scenes[0]->getTextures())
	{
		diit.emplace_back(texture.descriptor);
	}
	writes.emplace_back(_descSetLayoutBind.makeWriteArray(_descSet, SceneBindings::eTextures, diit.data()));

	// Writing the information
	vkUpdateDescriptorSets(Astra::Device::getInstance().getVkDevice(), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}

void Astra::DefaultApp::createRtDescriptorSet()
{
	const auto& device = Astra::Device::getInstance().getVkDevice();
	_rtDescSetLayoutBind.addBinding(RtxBindings::eTlas, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
	_rtDescSetLayoutBind.addBinding(RtxBindings::eOutImage, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR);

	_rtDescPool = _rtDescSetLayoutBind.createPool(device);
	_rtDescSetLayout = _rtDescSetLayoutBind.createLayout(device);

	VkDescriptorSetAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	allocateInfo.descriptorPool = _rtDescPool;
	allocateInfo.descriptorSetCount = 1;
	allocateInfo.pSetLayouts = &_rtDescSetLayout;
	vkAllocateDescriptorSets(device, &allocateInfo, &_rtDescSet);

	VkAccelerationStructureKHR tlas = ((SceneRT*)_scenes[0])->getTLAS();
	VkWriteDescriptorSetAccelerationStructureKHR descASInfo{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR };
	descASInfo.accelerationStructureCount = 1;
	descASInfo.pAccelerationStructures = &tlas;
	VkDescriptorImageInfo imageInfo{ {},_renderer->getOffscreenColor().descriptor.imageView, VK_IMAGE_LAYOUT_GENERAL};

	std::vector<VkWriteDescriptorSet> writes;
	writes.emplace_back(_rtDescSetLayoutBind.makeWrite(_rtDescSet, RtxBindings::eTlas, &descASInfo));
	writes.emplace_back(_rtDescSetLayoutBind.makeWrite(_rtDescSet, RtxBindings::eOutImage, &imageInfo));
	vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}

void Astra::DefaultApp::udpateRtDescriptorSet()
{
	VkDescriptorImageInfo imageInfo{ {}, _renderer->getOffscreenColor().descriptor.imageView, VK_IMAGE_LAYOUT_GENERAL };
	VkWriteDescriptorSet wds = _rtDescSetLayoutBind.makeWrite(_rtDescSet, RtxBindings::eOutImage, &imageInfo);
	vkUpdateDescriptorSets(Astra::Device::getInstance().getVkDevice(), 1, &wds, 0, nullptr);
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
	vkDeviceWaitIdle(Astra::Device::getInstance().getVkDevice());
	vkQueueWaitIdle(Astra::Device::getInstance().getQueue());

	// request swapchain image
	_renderer->requestSwapchainImage(w, h);

	_scenes[0]->getCamera()->setWindowSize(w, h);

	_renderer->createOffscreenRender();

	_renderer->updatePostDescriptorSet();

	_renderer->createDepthBuffer();
	_renderer->createFrameBuffers();

}

void Astra::DefaultApp::onMouseMotion(int x, int y)
{
}

void Astra::DefaultApp::onMouseButton(int button, int action, int mods)
{
}

void Astra::DefaultApp::onMouseWheel(int x, int y)
{
}

bool& Astra::DefaultApp::getUseRTref()
{
	return _useRT;
}
