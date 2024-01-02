#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <string>
#include <optional>

// function to get the address for vkCreateDebugUtilsMessengerEXT function, since its an extension function we first have to get the address
VkResult CreateDebutUtilsMessengerExt(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT  debugMessenger, const VkAllocationCallbacks* pAllocator);

const uint32_t WIDTH = 900;
const uint32_t HEIGHT = 1200;

const int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

// utils function
static std::vector<char> readFile(const std::string& filename);

/**
* Struct to store the different queueu family indices
*/
struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool isComplete() {
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

/**
* Struct storing the swapchain support details
*/
struct SwapchainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};


class HelloTriangleApplication {
public:
	/**
	* Application entrypoint: runs the app
	*/
	void run() {
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}

private:
	// window
	GLFWwindow* window;

	// vulkan instance
	VkInstance instance;

	// debug
	VkDebugUtilsMessengerEXT debugMessenger;

	// surface (screen area)
	VkSurfaceKHR surface;

	// devices
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device;

	// queues
	VkQueue graphicsQueue;
	VkQueue presentQueue;

	// swapchain
	VkSwapchainKHR swapchain;
	std::vector<VkImage> swapchainImages;
	VkFormat swapchainImageFormat;
	VkExtent2D swapchainExtent;
	// image views
	std::vector<VkImageView> swapchainImageViews;
	// framebuffer
	std::vector<VkFramebuffer> swapchainFramebuffers;

	// pipeline
	VkRenderPass renderPass;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;

	// command pools
	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> commandBuffers;

	// synchronization
	std::vector <VkSemaphore> imageAvailableSemaphores;
	std::vector <VkSemaphore> renderFinishedSemaphores;
	std::vector <VkFence> inFlightFences;

	// explicit resize
	bool framebufferResized = false;

	uint32_t currentFrame = 0;

	// -----------
	// METHODS
	// -----------

	/**
	* initializes the window where the app is going to be drawn to
	* uses GLFW
	*/
	void initWindow();

	/**
	* callback for the framebufferresized event in GLFW
	* switches the @e framebufferResized variable to true
	*  -- necessary for explicit resizing handling --
	*/
	static void framebufferResizedCallback(GLFWwindow* window, int width, int height);

	/*
	* Initializes the vulkan app, from "zero to heroe"
	*/
	void initVulkan();

	/*
	* main drawing loop
	*/
	void mainLoop();

	/*
	* objects cleanup
	*/
	void cleanup();

	/*
	* creates the vulkan instance
	*/
	void createInstance();

	/*
	* populates the createInfo struct needed for the debug messenger creation
	*/
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

	/**
	* setups the debug messenger
	* only when the validation layers are active
	*/
	void setupDebugMessenger();

	/**
	* creates the window surface from glfw
	*/
	void createSurface();

	/**
	* picks the physical device that the app will use
	* first queries vulkan support and then picks the best rated one
	*/
	void pickPhysicalDevice();

	/**
	* creates the logical device abstraction for the selected physical device
	*/
	void createLogicalDevice();

	/**
	* cleans up all the swapchain-related objects
	*/
	void cleanupSwapchain();

	/**
	* recreates the swapchain alongside the objects that depend on it
	*/
	void recreateSwapchain();

	/**
	* creates the swapchain
	*/
	void createSwapchain();

	/**
	* creates the image views for each of the swapchain images
	*/
	void createImageViews();

	/**
	* creates the renderpass with its subpasses
	*/
	void createRenderPass();

	/**
	* reads and creates the shadermodules for the current vertex and fragment shaders
	* creates all the fixed function stages of the render pipeline
	* finally creates the pipeline
	*/
	void createGraphicsPipeline();

	/**
	* creates framebuffers for each of the swapchain images
	*/
	void createFramebuffers();

	/**
	* creates the command pool
	*/
	void createCommandPool();

	/**
	* creates the command buffers
	*/
	void createCommandBuffers();

	/**
	* submits the commands to the command buffer
	* the actual "draw" function
	*/
	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

	/**
	* creates all the necessary sync objects:
	* semaphores for gpu sync
	* fences for cpu-gpu sync
	*/
	void createSyncObjects();

	/**
	* syncs, prepares and draws the frame
	*/
	void drawFrame();

	/**
	* creathes the shader module for a specified shader
	*/
	VkShaderModule createShaderModule(const std::vector<char>& code);

	/**
	* chooses the swapchain surface format
	* tries to pick VK_FORMAT_B8G8R8_SRGB and VK_COLOR_SPACE_SRGB_NONLINEAR_KHR values
	* if not, picks the first one availables
	*/
	VkSurfaceFormatKHR chooseSwapchainSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

	/**
	* picks the present mode for the swapchain
	* there are 4 modes: IMMEDIATE, FIFO, FIFO relaxed, MAILBOX
	* IMMEDIATE: images generated are transferred immediately => tearing
	* FIFO: image queue, if full => program waits for opening => vsync
	* FIFO relaxed: same as FIFO but if app is late and queue was empty at last vertical blank => immediate next image => tearing
	* MAILBOX: sames as FIFO, if full => last frames are switched for new ones => triple buffering
	*
	* We try to favour mailbox as the default presentmode
	* if mailbox is not present, FIFO is choosed
	*/
	VkPresentModeKHR chooseSwapchainPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

	/**
	*swapExtend is the resolution of the swapchain images, it should ~match the window resolution
	* VkSurfaceCapabilitesKHR provides the range of possible resolutions
	* we should set the current width and height in the currentExtent attribute
	* we are gonna set the values as the uint32_t limit and then choose the resolution that bests matches the window resolution
	*/
	VkExtent2D chooseSwapExtend(const VkSurfaceCapabilitiesKHR& capabilities);

	/**
	* since the swapchain is part of a vulkan extension, we need to query its support
	*/
	SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device);

	/**
	* gives points to the @param device physical device.
	* currently, it only values whether or not the gpu is a dedicated graphics card
	*/
	int rateDeviceSuitability(VkPhysicalDevice device);

	/**
	* checks if the extensions in @e deviceExtensions vector are supported by the device
	*/
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);

	/**
	* finds the queue families (graphic and present) for the @param device
	*/
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

	/**
	* gets the glfw-required extensions
	*/
	std::vector<const char*> getRequiredExtensions();

	/**
	* check it the validation layers in the @e validationLayers vector are supported
	*/
	bool checkValidationLayerSupport();

	/**
	* the debug callback for the debug messenger
	* prints the validation layer messages
	*/
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);
};