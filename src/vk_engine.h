// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <vk_types.h>
#include <deletion_queue.h>
#include <vk_descriptors.h>

struct FrameData {
	VkCommandPool _commandPool;
	VkCommandBuffer _mainCommandBuffer;
	VkSemaphore _swapchainSemaphore;
	VkFence _renderFence;
	DeletionQueue _deletionQueue;
};

constexpr unsigned int FRAME_OVERLAP = 2;

class VulkanEngine {
private:
	void init_vulkan();
	void init_swapchain();
	void init_commands();
	void init_sync_structures();
	void init_descriptors();
	void init_pipelines();
	void init_background_pipelines();

	void create_swapchain(uint32_t width, uint32_t height);
	void destroy_swapchain();

	void draw_background(VkCommandBuffer cmd);

public:
	VkInstance _instance;
	VkDebugUtilsMessengerEXT _debugMessenger;
	VkPhysicalDevice _physicalDevice;
	VkDevice _device;
	VkSurfaceKHR _surface;

	VkSwapchainKHR _swapchain;
	VkFormat _swapchainImageFormat;
	std::vector<VkImage> _swapchainImages;
	std::vector<VkImageView> _swapchainImageViews;
	VkExtent2D _swapchainExtent;

	AllocatedImage _drawImage;
	VkExtent2D _drawExtent;

	DescriptorAllocator globalDescriptorAllocator;
	VkDescriptorSet _drawImageDescriptors;
	VkDescriptorSetLayout _drawImageDescriptorLayout;

	VkPipeline _gradientPipeline;
	VkPipelineLayout _gradientPipelineLayout;

	std::vector<VkSemaphore> _renderSemaphores;

	FrameData _frames[FRAME_OVERLAP];

	FrameData& get_current_frame() { return _frames[_frameNumber % FRAME_OVERLAP]; }

	VkQueue _graphicsQueue;
	uint32_t _graphicsQueueFamily;

	VmaAllocator _allocator;
	DeletionQueue _deletionQueue;

	bool _isInitialized{ false };
	int _frameNumber {0};
	bool stop_rendering{ false };
	VkExtent2D _windowExtent{ 1280 , 720 };

	struct SDL_Window* _window{ nullptr };

	static VulkanEngine& Get();

	//initializes everything in the engine
	void init();

	//shuts down the engine
	void cleanup();

	//draw loop
	void draw();

	//run main loop
	void run();

	VulkanEngine() = default;
	VulkanEngine(const VulkanEngine&) = delete;
	VulkanEngine& operator=(const VulkanEngine&) = delete;
	VulkanEngine(VulkanEngine&&) = delete;
	VulkanEngine& operator=(VulkanEngine&&) = delete;
};
