#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <map>
#include <optional>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

VkResult CreateDebutUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

// clean up
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT callback, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, callback, pAllocator);
	}
}

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;

	bool isComplete() {
		return graphicsFamily.has_value();
	}
};

class HelloTriangleApplication {
public:
	void run() {
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}
private:
	GLFWwindow* window;
	vk::UniqueInstance instance;
	VkDebugUtilsMessengerEXT callback;
	vk::PhysicalDevice physicalDevice;
	vk::UniqueDevice device;
	vk::Queue graphicsQueue;
	vk::SurfaceKHR surface;


	void initWindow() {
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
	}

	void initVulkan() {
		createInstance();
		setupDebugCallback();
		//createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
	}

	void mainLoop() {
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
		}
	}

	void cleanup() {
		// NOTE: instance destruction is handled by UniqueInstance, same for device

		instance->destroySurfaceKHR(surface);

		if (enableValidationLayers) {
			DestroyDebugUtilsMessengerEXT(*instance, callback, nullptr);
		}

		glfwDestroyWindow(window);

		glfwTerminate();
	}

	void createInstance() {
		if (enableValidationLayers && !checkValidationLayerSupport()) {
			throw std::runtime_error("validation layers requestes, but not available!");
		}

		vk::ApplicationInfo appInfo = vk::ApplicationInfo(
			"Hello Triangle",
			VK_MAKE_VERSION(1, 0, 0),
			"No Engine",
			VK_MAKE_VERSION(1, 0, 0),
			vk::ApiVersion10
		);

		auto extensions = getRequiredExtensions();
		vk::InstanceCreateInfo createInfo = vk::InstanceCreateInfo(vk::InstanceCreateFlags(), &appInfo, 0, nullptr, static_cast<uint32_t>(extensions.size()), extensions.data());

		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}

		try {
			instance = vk::createInstanceUnique(createInfo, nullptr);
		}
		catch (vk::SystemError err) {
			throw std::runtime_error("failed to create instance!");
		}

	}

	void setupDebugCallback() {
		if (!enableValidationLayers) return;
		vk::DebugUtilsMessengerCreateInfoEXT createInfo(
			vk::DebugUtilsMessengerCreateFlagsEXT(),
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
			vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
			debugCallback,
			nullptr
		);

		//instance->createDebugUtilsMessengerEXT(createInfo);
		//instance->createDebugUtilsMessengerEXTUnique(createInfo);

		// NOTE: reinterpret_cast is also used by vulkan.hpp internally for all these structs
		if (CreateDebutUtilsMessengerEXT(*instance, reinterpret_cast<const VkDebugUtilsMessengerCreateInfoEXT*>(&createInfo), nullptr, &callback) != VK_SUCCESS) {
			throw std::runtime_error("failed to set up debug callback!");
		}
	}

	void pickPhysicalDevice() {
		auto devices = instance->enumeratePhysicalDevices();
		if (devices.size() == 0) {
			throw std::runtime_error("no gpus with vulkan support found!");
		}

		std::multimap<int, vk::PhysicalDevice> candidates;

		for (const auto& device : devices) {
			int score = rateDeviceSuitability(device);
			candidates.insert(std::make_pair(score, device));
		}

		if (candidates.rbegin()->first > 0) {
			physicalDevice = candidates.rbegin()->second;
		}
		else {
			throw std::runtime_error("failed to find any suitable gpu");
		}
	}

	int rateDeviceSuitability(vk::PhysicalDevice device) {
		QueueFamilyIndices indices = findQueueFamilies(device);
		vk::PhysicalDeviceProperties deviceProperties = device.getProperties();
		vk::PhysicalDeviceFeatures deviceFeatures = device.getFeatures();
		int score = 0;
		if (deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
			score += 1000;
		}
		score += deviceProperties.limits.maxImageDimension2D;

		if (!deviceFeatures.geometryShader || !indices.isComplete()) {
			score = 0;
		}
		std::cout << deviceProperties.deviceName << ": " << score << std::endl;
		return score;
	}

	QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device) {
		QueueFamilyIndices indices;
		std::vector<vk::QueueFamilyProperties> queueFamilies = device.getQueueFamilyProperties();

		int i = 0;
		for (const auto& queueFamily : queueFamilies) {
			if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
				indices.graphicsFamily = i;
			}
			if (indices.isComplete()) {
				break;
			}
			i++;
		}
		return indices;
	}

	void createLogicalDevice() {
		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
		float queuePriority = 1.0f;
		vk::DeviceQueueCreateInfo queueCreateInfo = vk::DeviceQueueCreateInfo(
			vk::DeviceQueueCreateFlags(),
			indices.graphicsFamily.value(),
			1, // queueCount
			&queuePriority
		);

		vk::PhysicalDeviceFeatures deviceFeatures = physicalDevice.getFeatures();

		vk::DeviceCreateInfo createInfo = vk::DeviceCreateInfo(
			vk::DeviceCreateFlags(),
			1, &queueCreateInfo
		);
		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.enabledExtensionCount = 0;

		createInfo.enabledExtensionCount = 0;
		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else {
			createInfo.enabledLayerCount = 0;
		}

		try {
			device = physicalDevice.createDeviceUnique(createInfo);
		}
		catch (vk::SystemError err) {
			throw std::runtime_error("Failed to create logical device!");
		}
		graphicsQueue = device->getQueue(indices.graphicsFamily.value(), 0);
	}

	std::vector<const char*> getRequiredExtensions() {
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		if (enableValidationLayers) {
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}

	bool checkValidationLayerSupport() {
		auto availableLayers = vk::enumerateInstanceLayerProperties();

		for (const char* layerName : validationLayers) {
			bool layerFound = false;
			for (const auto& layerProperties : availableLayers) {
				if (strcmp(layerName, layerProperties.layerName) == 0) {
					layerFound = true;
					break;
				}
			}

			if (!layerFound) {
				return false;
			}
		}

		return true;
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
	{
		std::string severity;
		if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
			severity = "VERBOSE";
		else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
			severity = "WARNING";
		else
			severity = "ERROR";

		std::cerr << "[" << severity << "]" << " validation layer : " << pCallbackData->pMessage << std::endl;

		return VK_FALSE;
	}
};

int main() {
	HelloTriangleApplication app;

	try {
		app.run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}