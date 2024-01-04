#include "VulkanApp.h"
#include <stdexcept>
#include <map>
#include <set>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>



std::vector<char> readFile(const std::string& filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary); // start reading at the end so we now file size
	if (!file.is_open()) {
		throw std::runtime_error("failed to open file! (" + filename + ")");
	}
	// allocate for correct file size
	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);
	file.seekg(0); // back to the begginning to read bytes
	file.read(buffer.data(), fileSize);
	std::cout << filename << ": " << fileSize << " bytes" << std::endl;
	file.close();
	return buffer;
}

void VulkanApp::run()
{
	initWindow();
	initVulkan();
	mainLoop();
	cleanup();
}

void VulkanApp::initWindow()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan App", nullptr, nullptr);

	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, framebufferResizedCallback);
}

void VulkanApp::framebufferResizedCallback(GLFWwindow* window, int width, int height)
{
	auto app = reinterpret_cast<VulkanApp*> (glfwGetWindowUserPointer(window));
	app->framebufferResized = true;
}

void VulkanApp::initVulkan()
{
	createInstance();
	setupDebugMessenger();
	createSurface();
	pickPhysicalDevice();
	createLogicalDevice();
	createSwapchain();
	createImageViews();
	createRenderPass();
	createDescriptorSetLayout();
	createGraphicsPipeline();
	createFramebuffers();
	createCommandPool();
	createVertexBuffer();
	createIndexBuffer();
	createUniformBuffers();
	createDescriptorPool();
	createDescriptorSets();
	createCommandBuffers();
	createSyncObjects();
}

void VulkanApp::mainLoop()
{
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		drawFrame();
	}
	device->waitIdle();
}

void VulkanApp::cleanup()
{
	cleanupSwapchain();

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		device->destroyBuffer(uniformBuffers[i], nullptr);
		device->freeMemory(uniformBuffersMemory[i], nullptr);
	}

	device->destroyDescriptorPool(descriptorPool, nullptr);
	device->destroyDescriptorSetLayout(descriptorSetLayout, nullptr);

	device->destroyBuffer(vertexBuffer, nullptr);
	device->freeMemory(vertexBufferMemory, nullptr);
	device->destroyBuffer(indexBuffer, nullptr);
	device->freeMemory(indexBufferMemory, nullptr);

	device->destroyPipeline(graphicsPipeline, nullptr);
	device->destroyPipelineLayout(pipelineLayout, nullptr);
	device->destroyRenderPass(renderPass, nullptr);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		device->destroySemaphore(renderFinishedSemaphores[i], nullptr);
		device->destroySemaphore(imageAvailableSemaphores[i], nullptr);
		device->destroyFence(inFlightFences[i], nullptr);
	}

	device->destroyCommandPool(commandPool, nullptr);

	device->destroy();

	instance->destroySurfaceKHR(surface, nullptr);
	instance->destroy();

	glfwDestroyWindow(window);

	glfwTerminate();
}

void VulkanApp::createInstance()
{
	if (enableValidationLayers && !checkValidationLayerSupport()) {
		throw std::runtime_error("validation layers requested, but not available!");
	}

	vk::ApplicationInfo appInfo = vk::ApplicationInfo(
		"Vulkan App",
		VK_MAKE_VERSION(1, 0, 0),
		"No Engine",
		VK_MAKE_VERSION(1, 0, 0),
		vk::ApiVersion10
	);

	auto createInfo = vk::InstanceCreateInfo(vk::InstanceCreateFlags(), &appInfo);

	auto extensions = getRequiredExtensions();
	createInfo.enabledExtensionCount = static_cast<uint32_t> (extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t> (validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();

		auto debugCreateInfo = populateDebugMessengerCreateInfo();
		createInfo.pNext = (vk::DebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}
	else {
		createInfo.enabledExtensionCount = 0;
		createInfo.pNext = nullptr;
	}

	instance = vk::createInstanceUnique(createInfo);
}

vk::DebugUtilsMessengerCreateInfoEXT VulkanApp::populateDebugMessengerCreateInfo()
{
	auto severityFlags = vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
		| vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
		| vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
		| vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo;

	auto typeFlags = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
		| vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
		| vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;

	vk::DebugUtilsMessengerCreateInfoEXT messengerCreateInfo;
	messengerCreateInfo.flags = vk::DebugUtilsMessengerCreateFlagBitsEXT();
	messengerCreateInfo.messageSeverity = severityFlags;
	messengerCreateInfo.messageType = typeFlags;
	messengerCreateInfo.pfnUserCallback = debugCallback;

	return messengerCreateInfo;
}

void VulkanApp::setupDebugMessenger()
{
	auto createInfo = populateDebugMessengerCreateInfo();
	vk::DispatchLoaderDynamic instanceLoader(*instance, vkGetInstanceProcAddr);
	debugMessenger = instance->createDebugUtilsMessengerEXT(createInfo, nullptr, instanceLoader);
}

void VulkanApp::createSurface()
{
	VkSurfaceKHR surf;
	if (glfwCreateWindowSurface(static_cast<VkInstance>(*instance), window, nullptr, &surf) != VK_SUCCESS) {
		throw std::runtime_error("failed to create surface");
	}
	surface = surf;
}

void VulkanApp::pickPhysicalDevice()
{
	auto physicalDevices = instance->enumeratePhysicalDevices();
	if (physicalDevices.size() == 0) {
		throw std::runtime_error("failed to find any GPUs with vulkan support!");
	}
	std::multimap<int, vk::PhysicalDevice> candidates;

	for (const auto& device : physicalDevices) {
		int score = rateDeviceSuitability(device);
		candidates.insert(std::make_pair(score, device));
	}

	if (candidates.rbegin()->first > 0) {
		physicalDevice = candidates.rbegin()->second;
	}
	else {
		throw std::runtime_error("failed to find any suitable GPU");
	}
}

void VulkanApp::createLogicalDevice()
{
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
	std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		vk::DeviceQueueCreateInfo queueCreateInfo(
			{}, // flags
			queueFamily, // queueFamilyIndex
			1, // queueCount
			&queuePriority); // pQueuePriorities
		queueCreateInfos.push_back(queueCreateInfo);
	}

	vk::DeviceCreateInfo createInfo({},
		queueCreateInfos.size(), // queueCreateInfoCount
		queueCreateInfos.data(), // pQueueCreateInfos
		enableValidationLayers ? static_cast<uint32_t> (validationLayers.size()) : 0, // layer count
		enableValidationLayers ? validationLayers.data() : nullptr, // layer ptr
		static_cast<uint32_t> (deviceExtensions.size()), // extensions count
		deviceExtensions.data() // extensions ptr
	);

	device = physicalDevice.createDeviceUnique(createInfo);
	graphicsQueue = device->getQueue(indices.graphicsFamily.value(), 0);
	presentQueue = device->getQueue(indices.presentFamily.value(), 0);
}

void VulkanApp::cleanupSwapchain()
{
	for (auto fb : swapchainFramebuffers) {
		device->destroyFramebuffer(fb, nullptr);
	}

	for (auto iv : swapchainImageViews) {
		device->destroyImageView(iv, nullptr);
	}

	device->destroySwapchainKHR(swapchain, nullptr);
}

void VulkanApp::recreateSwapchain()
{
	// handling minimization (width and height are 0)
	int width = 0, height = 0;
	glfwGetFramebufferSize(window, &width, &height);
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(window, &width, &height);
		glfwWaitEvents();
	}

	device->waitIdle();
	cleanupSwapchain();

	createSwapchain();
	createImageViews();
	createFramebuffers();
}

void VulkanApp::createSwapchain()
{
	SwapchainSupportDetails swapchainSupport = querySwapchainSupport(physicalDevice);

	vk::SurfaceFormatKHR surfaceFormat = chooseSwapchainSurfaceFormat(swapchainSupport.formats);
	vk::PresentModeKHR presentMode = chooseSwapchainPresentMode(swapchainSupport.presentModes);
	vk::Extent2D extent = chooseSwapExtend(swapchainSupport.capabilities);

	// at least 1 more than the minimun so there is less idle time
	uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;

	// 0 is a special value that means that there is no maximum
	if (swapchainSupport.capabilities.maxImageCount > 0 && imageCount > swapchainSupport.capabilities.maxImageCount) {
		imageCount = swapchainSupport.capabilities.maxImageCount;
	}

	vk::SwapchainCreateInfoKHR createInfo(
		{}, // flags
		surface, // surface
		imageCount, // minImageCount
		surfaceFormat.format, // format
		surfaceFormat.colorSpace, // colorSpace
		extent, // extent
		1, // image array layers
		/*
		* This bit specifies what kind of operation are the images generated in the swapchain used for.
		* As of now, the images are going to be rendered directly into them => they are used as color attachment
		* We could render images to a separate image first to perform operations like post-processing
		* In that case, maybe using VK_IMAGE_USAGE_TRANSFER_DST_BIT
		* and then use a memory operation to transfer the rendered image to the swapchain
		*/
		vk::ImageUsageFlagBits::eColorAttachment // image usage
	);

	/*
	* We need to specify how to handle swapchain images in the different queue families
	* We will draw the image in the swapchain from the graphics queue and then submit it to the presentation queue
	*
	* There are two ways to handle that:
	* * CONCURRENT: images can be used across multiple queues without explicit ownership transfers
	* * EXCLUSIVE: an image is owned by one queue family at a time. Ownership must be explicitly transferred. Better performance
	*
	* As of now, we'll stick to concurrent so that we don't have to handle ownership transfers
	*/
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	if (indices.graphicsFamily != indices.presentFamily) {
		createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		createInfo.imageSharingMode = vk::SharingMode::eExclusive;
		createInfo.queueFamilyIndexCount = 0; // Optional
		createInfo.pQueueFamilyIndices = nullptr; // Optional
	}

	createInfo.preTransform = swapchainSupport.capabilities.currentTransform; // no transformations

	createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;

	createInfo.presentMode = presentMode;
	createInfo.clipped = vk::True;

	createInfo.oldSwapchain = nullptr; // Future

	swapchain = device->createSwapchainKHR(createInfo);
	swapchainImages = device->getSwapchainImagesKHR(swapchain);

	swapchainImageFormat = surfaceFormat.format;
	swapchainExtent = extent;

}

void VulkanApp::createImageViews()
{
	swapchainImageViews.resize(swapchainImages.size());

	for (size_t i = 0; i < swapchainImageViews.size(); i++) {
		vk::ImageViewCreateInfo createInfo(
			{}, // flags
			swapchainImages[i], // image
			vk::ImageViewType::e2D, // viewType
			swapchainImageFormat, // format
			{
				vk::ComponentSwizzle::eIdentity, // components.r
				vk::ComponentSwizzle::eIdentity, // components.g
				vk::ComponentSwizzle::eIdentity, // components.b
				vk::ComponentSwizzle::eIdentity  // components.a
			},
			{
				vk::ImageAspectFlagBits::eColor, // subresourceRange.aspectMask
				0, // subresourceRange.baseMipmaplevel
				1, // subresourceRange.levelCount
				0, // subresourceRange.baseArrayLayer
				1 // subresourceRange.layerCount
			}
			);
		swapchainImageViews[i] = device->createImageView(createInfo);
	}
}

void VulkanApp::createRenderPass()
{
	vk::AttachmentDescription colorAttachment(
		{}, // flags
		swapchainImageFormat, // format
		vk::SampleCountFlagBits::e1, // samples
		vk::AttachmentLoadOp::eClear, // load op
		vk::AttachmentStoreOp::eStore, // store op
		vk::AttachmentLoadOp::eDontCare, // stencil load op
		vk::AttachmentStoreOp::eDontCare, // stencil store op
		vk::ImageLayout::eUndefined, // initial layout
		vk::ImageLayout::ePresentSrcKHR // final layout
	);

	vk::AttachmentReference colorAttachmentRef(
		0, // attachment -> index of attachment, since we only have 1 its 0
		vk::ImageLayout::eColorAttachmentOptimal // layout
	);

	vk::SubpassDescription subpass;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	// make the renderpass wait until image is acquired
	vk::SubpassDependency dependency;
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	dependency.srcAccessMask = {};
	dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

	vk::RenderPassCreateInfo createInfo(
		{}, // flags
		1, // attachment count
		&colorAttachment, // attachment ptr
		1, // subpass count
		&subpass, // subpass ptr
		1, // dependency count
		&dependency // dependency ptr
	);

	renderPass = device->createRenderPass(createInfo);
}

void VulkanApp::createDescriptorSetLayout()
{
	vk::DescriptorSetLayoutBinding uboLayoutBinding(
		0, // binding -> (binding = 0) in shader
		vk::DescriptorType::eUniformBuffer, // descriptor type
		1, // descriptor count
		vk::ShaderStageFlagBits::eVertex, // stage flags
		nullptr // pImmutableSamplers
	);

	vk::DescriptorSetLayoutCreateInfo layoutInfo(
		{}, // flags
		1, // binding count
		&uboLayoutBinding // pBindings
	);

	descriptorSetLayout = device->createDescriptorSetLayout(layoutInfo);
}

void VulkanApp::createGraphicsPipeline()
{
	// read the bytecode
	auto vertShaderCode = readFile("shaders/shader_vert.spv");
	auto fragShaderCode = readFile("shaders/shader_frag.spv");

	// wrap it in vk::ShaderModule 
	vk::ShaderModule vertShaderModule = createShaderModule(vertShaderCode);
	vk::ShaderModule fragShaderModule = createShaderModule(fragShaderCode);

	// create the shader stages for each shader
	vk::PipelineShaderStageCreateInfo vertShaderStageInfo(
		{}, // flags
		vk::ShaderStageFlagBits::eVertex, // stage
		vertShaderModule, // module
		"main" // pName
	);

	vk::PipelineShaderStageCreateInfo fragShaderStageInfo(
		{}, // flags
		vk::ShaderStageFlagBits::eFragment, // stage
		fragShaderModule, // module
		"main" // pName
	);

	vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	// fixed functions

	auto bindingDescriptions = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();

	vk::PipelineVertexInputStateCreateInfo vertexInputInfo(
		{}, // flags
		1, // vertexBindingDescription count
		&bindingDescriptions, // vertexBindingDescription ptr
		static_cast<uint32_t>(attributeDescriptions.size()), // vertexAttributeDescription count
		attributeDescriptions.data() // vertexAttributeDescription ptr
	);

	// Input assembly (geometry from vertices[primitives])
	/*
	* (some) Possible options
	* ePointList -> points
	* eLineList -> lines (no reuse)
	* eLineStrip -> lines (reusing end -> beginning)
	* eTriangleList -> triangle from every 3 vertices || classic
	* eTriangleStrip -> triangle strip
	*/
	vk::PipelineInputAssemblyStateCreateInfo inputAssembly(
		{}, // flags
		vk::PrimitiveTopology::eTriangleList, // topology
		vk::False // primite restart enablk
	);

	vk::Viewport viewport(
		0.0f, 0.0f, (float)swapchainExtent.width, (float)swapchainExtent.height, 0.0f, 1.0f // x, y, width, height, minDepth, maxDepth
	);

	vk::Rect2D scissor(
		{ 0,0 }, swapchainExtent // offset, extent
	);

	// we are going to have a dynamic state so that we can change the viewport or scissor size during runtime
	std::vector<vk::DynamicState> dynamicStates = {
		vk::DynamicState::eViewport, vk::DynamicState::eScissor
	};

	vk::PipelineDynamicStateCreateInfo dynamicState(
		{}, // flags
		static_cast<uint32_t>(dynamicStates.size()), // dynamic states count
		dynamicStates.data() // dynamic states ptr
	);

	vk::PipelineViewportStateCreateInfo viewportState(
		{}, // flags
		1, //viewport count
		nullptr, // viewport ptr (nullptr as we'll set it dynamically)
		1, // scissor count
		nullptr // scissor ptr (nullptr as we'll set it dynamically)
	);

	vk::PipelineRasterizationStateCreateInfo rasterizer(
		{}, // flags
		vk::False, // depthClampEnable -> clamps objects outside of near/far to these planes
		vk::False, // rasterizerDiscardEnable -> if true no geometry passes through the rasterizer
		vk::PolygonMode::eFill, // polygonMode
		vk::CullModeFlagBits::eBack, // cullMode
		vk::FrontFace::eCounterClockwise, // frontFace
		vk::False, // deptBiasEnable
		0.0f, // depthBiasConstantFactor
		0.0f, // depthBiasClamp
		0.0f, // depthBiasSlopeFactor
		1.0f // lineWidth
	);

	vk::PipelineMultisampleStateCreateInfo multisampling(
		{}, // flags
		vk::SampleCountFlagBits::e1, // rasterization samples
		vk::False, // sampleShadingEnable
		1.0f, // minSampleShading
		nullptr, // pSampleMask
		vk::False, // alphatoOneCoverageEnable
		vk::False // alphatoOneEnable
	);

	// depth and stencil test
	// currently disabled

	// color blending
	// attachmentState-> per framebuffer
	// createinfo -> global
	vk::PipelineColorBlendAttachmentState colorBlendAttachment(vk::False);
	colorBlendAttachment.colorWriteMask = vk::ColorComponentFlags();

	vk::PipelineColorBlendStateCreateInfo colorBlending(
		{}, // flags
		vk::False, // logicOpEnable
		vk::LogicOp::eCopy, // logicOp
		1, // attachment count
		&colorBlendAttachment, // attachment ptr
		{
			0.0f, 0.0f, 0.0f, 0.0f // blend constants
		}
	);

	// pipeline layout
	vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo(
		{}, // flags
		1, // setlayout count
		&descriptorSetLayout // setlayout ptr
	);

	pipelineLayout = device->createPipelineLayout(pipelineLayoutCreateInfo);

	vk::GraphicsPipelineCreateInfo pipelineInfo(
		{}, // flags
		2, // stage count
		shaderStages, // shader stages
		&vertexInputInfo, // vertex input state ptr
		&inputAssembly, // input assembly state ptr
		nullptr, // tesselation state ptr
		&viewportState, // viewport state ptr
		&rasterizer, // rasterization state ptr
		&multisampling, // multisample state ptr
		nullptr, // depth stencil state ptr
		&colorBlending, // color blend state ptr
		&dynamicState, // dynamic state ptr
		pipelineLayout, // layout
		renderPass, // renderpass
		0, // subpass
		nullptr, // basepipelinehandle
		-1 // basePipelineindex
	);

	graphicsPipeline = device->createGraphicsPipelines({}, pipelineInfo).value[0];

	device->destroyShaderModule(vertShaderModule);
	device->destroyShaderModule(fragShaderModule);
}

void VulkanApp::createFramebuffers()
{
	swapchainFramebuffers.resize(swapchainImageViews.size());

	for (size_t i = 0; i < swapchainImageViews.size(); i++) {
		vk::ImageView attachments[] = { swapchainImageViews[i] };
		vk::FramebufferCreateInfo createInfo(
			{}, // flags
			renderPass, // renderpass
			1, // attachment count
			attachments, // attachments ptr
			swapchainExtent.width, // width
			swapchainExtent.height, // height
			1
		);
		swapchainFramebuffers[i] = device->createFramebuffer(createInfo);
	}
}

void VulkanApp::createCommandPool()
{
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);
	vk::CommandPoolCreateInfo poolInfo(
		vk::CommandPoolCreateFlagBits::eResetCommandBuffer, // flag
		queueFamilyIndices.graphicsFamily.value() // queueFamilyIndex
	);
	commandPool = device->createCommandPool(poolInfo);
}

void VulkanApp::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Buffer& buffer, vk::DeviceMemory& bufferMemory)
{
}

void VulkanApp::copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size)
{
}

void VulkanApp::createVertexBuffer()
{
}

void VulkanApp::createIndexBuffer()
{
}

void VulkanApp::createUniformBuffers()
{
}

void VulkanApp::createDescriptorPool()
{
}

void VulkanApp::createDescriptorSets()
{
}

void VulkanApp::updateUniformBuffer(uint32_t currentImage)
{
}

uint32_t VulkanApp::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
{
	return 0;
}

void VulkanApp::createCommandBuffers()
{
}

void VulkanApp::recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex)
{
}

void VulkanApp::createSyncObjects()
{
}

void VulkanApp::drawFrame()
{
}

vk::ShaderModule VulkanApp::createShaderModule(const std::vector<char>& code)
{
	return vk::ShaderModule();
}

vk::SurfaceFormatKHR VulkanApp::chooseSwapchainSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
{
	return vk::SurfaceFormatKHR();
}

vk::PresentModeKHR VulkanApp::chooseSwapchainPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
{
	return vk::PresentModeKHR();
}

vk::Extent2D VulkanApp::chooseSwapExtend(const vk::SurfaceCapabilitiesKHR& capabilities)
{
	return vk::Extent2D();
}

SwapchainSupportDetails VulkanApp::querySwapchainSupport(vk::PhysicalDevice device)
{
	return SwapchainSupportDetails();
}

int VulkanApp::rateDeviceSuitability(vk::PhysicalDevice device)
{
	return 0;
}

bool VulkanApp::checkDeviceExtensionSupport(vk::PhysicalDevice device)
{
	return false;
}

QueueFamilyIndices VulkanApp::findQueueFamilies(vk::PhysicalDevice device)
{
	return QueueFamilyIndices();
}

std::vector<const char*> VulkanApp::getRequiredExtensions()
{
	return std::vector<const char*>();
}

bool VulkanApp::checkValidationLayerSupport()
{
	return false;
}


// could only get it to work with original C API, no C++ bindings
VKAPI_ATTR VkBool32 VKAPI_CALL VulkanApp::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	std::string severity;
	if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
		severity = "VERBOSE";
	else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		severity = "WARNING";
	else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
		severity = "INFO";
	else
		severity = "ERROR";

	std::cerr << "[" << severity << "]" << " validation layer : " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}