#include <Device.h>
#include <nvvk/context_vk.hpp>
#include <stdexcept>

constexpr auto SAMPLE_WIDTH = 1280;
constexpr auto SAMPLE_HEIGHT = 720;

namespace Astra {

	// if the struct has any non-empty array we assume that it is and advanced
	// user and will provide every extensions or instance layer needed

	void Device::initDevice(DeviceCreateInfo createInfo) {
		// fill createInfo with default values if empty
		if (createInfo.instanceLayers.empty()) {
			if (createInfo.debug) {
				createInfo.instanceLayers.push_back("VK_LAYER_KHRONOS_validation");
				createInfo.instanceLayers.push_back("VK_LAYER_LUNARG_monitor");
			}
		}

		if (createInfo.instanceExtensions.empty()) {
			uint32_t count{ 0 };
			auto glfwExtensions = glfwGetRequiredInstanceExtensions(&count);

			createInfo.instanceExtensions.resize(count);

			for (uint32_t i = 0; i < count; i++) {
				createInfo.instanceExtensions[i] = glfwExtensions[i];
			}

			if (createInfo.debug) {
				createInfo.instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			}
		}

		if (createInfo.deviceExtensions.empty()) {
			createInfo.deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
			//createInfo.deviceExtensions.push_back(VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME); // enable shader printf
		}
		// create glfw window
		if (!glfwInit()) {
			throw std::runtime_error("Error initializing glfw");
		}
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		_window = glfwCreateWindow(SAMPLE_WIDTH, SAMPLE_HEIGHT, "Astra App", nullptr, nullptr);

		nvvk::ContextCreateInfo contextInfo;
		contextInfo.setVersion(createInfo.vkVersionMajor, createInfo.vkVersionMinor);
		for (const auto& layer : createInfo.instanceLayers) {
			contextInfo.addInstanceLayer(layer.data(), true);
		}
		for (const auto& ext : createInfo.instanceExtensions) {
			contextInfo.addInstanceExtension(ext.data(), true);
		}
		for (const auto& ext : createInfo.deviceExtensions) {
			contextInfo.addDeviceExtension(ext.data(), true);
		}

		nvvk::Context vkctx{};
		if (!vkctx.initInstance(contextInfo)) {
			throw std::runtime_error("Error creating instance!");
		}

		auto compatibleDevices = vkctx.getCompatibleDevices(contextInfo);
		if (compatibleDevices.empty()) {
			throw std::runtime_error("No valid GPU was found");
		}

		if (!vkctx.initDevice(compatibleDevices[0], contextInfo)) {
			throw std::runtime_error("Error initiating device!");
		}

		// filling the data
		_instance = vkctx.m_instance;
		_vkdevice = vkctx.m_device;
		_physicalDevice = vkctx.m_physicalDevice;
		_graphicsQueueIndex = vkctx.m_queueGCT.familyIndex;
		vkGetDeviceQueue(_vkdevice, _graphicsQueueIndex, 0, &_queue);

		// getting the surface

		VkSurfaceKHR surface{};
		if (glfwCreateWindowSurface(_instance, _window, nullptr, &surface) != VK_SUCCESS) {
			throw std::runtime_error("Error creating window surface");
		}
		_surface = surface;

		// will throw exception if not supported
		VkBool32 supported;
		vkGetPhysicalDeviceSurfaceSupportKHR(_physicalDevice, _graphicsQueueIndex, _surface, &supported);
		if (supported != VK_TRUE) {
			throw std::runtime_error("Error, device does not support presenting");
		}

		VkCommandPoolCreateInfo poolCreateInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		vkCreateCommandPool(_vkdevice, &poolCreateInfo, nullptr, &_cmdPool);

		_alloc.init(_instance, _vkdevice, _physicalDevice);
		_debug.setup(_vkdevice);
	}

	VkShaderModule Device::createShaderModule(const std::vector<char>& code)
	{
		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.flags = {};
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule = VK_NULL_HANDLE;
		if (vkCreateShaderModule(_vkdevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
		{
			throw std::runtime_error("Error creating shader module");
		}

		return shaderModule;
	}

	GLFWwindow* Device::getWindow()
	{
		return _window;
	}

	VkCommandBuffer Device::createTmpCmdBuf()
	{
		// allocate 
		VkCommandBufferAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		allocateInfo.commandBufferCount = 1;
		allocateInfo.commandPool = _cmdPool;
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		
		// create
		VkCommandBuffer cmdBuffer;
		vkAllocateCommandBuffers(_vkdevice, &allocateInfo, &cmdBuffer);
		
		// start
		VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(cmdBuffer, &beginInfo);
		return cmdBuffer;
	}

	void Device::submitTmbCmdBuf(VkCommandBuffer cmdBuff)
	{
		// end
		vkEndCommandBuffer(cmdBuff);
		// submit
		VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmdBuff;
		vkQueueSubmit(_queue, 1, &submitInfo, {});
		// sync
		vkQueueWaitIdle(_queue);
		// free
		vkFreeCommandBuffers(_vkdevice, _cmdPool, 1, &cmdBuff);
	}
	
	VkInstance Device::getVkInstance() const
	{
		return _instance;
	}
	
	VkDevice Device::getVkDevice() const
	{
		return _vkdevice;
	}
	VkSurfaceKHR Device::getSurface() const
	{
		return _surface;
	}
	
	VkPhysicalDevice Device::getPhysicalDevice() const
	{
		return _physicalDevice;
	}

	VkQueue Device::getQueue() const
	{
		return _queue;
	}

	uint32_t Device::getGraphicsQueueIndex() const 
	{
		return _graphicsQueueIndex;
	}
	VkCommandPool Device::getCommandPool() const
	{
		return _cmdPool;
	}

}