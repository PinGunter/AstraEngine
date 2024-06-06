#pragma once
#include <Pipeline.h>
#include <Scene.h>
#include <App.h>
#include <nvvk/swapchain_vk.hpp>
#include <GLFW/glfw3.h>

namespace Astra {
	class Renderer {
	protected:
		PostPipeline _postPipeline;
		VkRenderPass _postRenderPass;
		nvvk::SwapChain _swapchain;
		std::vector<VkFramebuffer> _framebuffers;
		std::vector<VkCommandBuffer> _commandBuffers;
		std::vector<VkFence> _fences;
		VkImage _depthImage;
		VkDeviceMemory _depthMemory;
		VkImageView _depthView;
		VkExtent2D _size{ 0,0 };
		VkFormat _colorFormat{ VK_FORMAT_B8G8R8A8_UNORM };
		VkFormat _depthFormat{ VK_FORMAT_UNDEFINED };

		std::vector<Pipeline* > _pipelines; // in this case will use 0 for rt, 1 for raster

		void renderPost(); // mandatory step! after drawing
		void renderRaster(const Scene& scene, RasterPipeline* pipeline);
		void renderRaytrace(const Scene& scene, RayTracingPipeline* pipeline);

		void createSwapchain(const VkSurfaceKHR& surface, uint32_t width, uint32_t height, VkFormat colorFormat, VkFormat depthFormat);
		void resize(int w, int h);
		void prepareFrame();
		void endFrame();
	public:
		void init(App* app);
		void render(const Scene & scene, Pipeline* pipeline);
	};
}