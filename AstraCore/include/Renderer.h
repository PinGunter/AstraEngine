#pragma once
#include <Pipeline.h>
#include <Scene.h>
#include <App.h>
#include <nvvk/swapchain_vk.hpp>
#include <GLFW/glfw3.h>
#include <CommandList.h>
#include <GuiController.h>

namespace Astra {
	class App;
	class GuiController;

	// the default renderer uses an offscreen renderpass to a texture
	// then renders that texture as a fullscreen triangle
	// this allows us to mix raytracing and rasterization
	// main use case is raytracing the scene and then rasterizing the ui
	class Renderer {
	protected:
		// offscreen
		VkRenderPass _offscreenRenderPass;
		VkFramebuffer _offscreenFb;
		nvvk::Texture _offscreenColor;
		nvvk::Texture _offscreenDepth;
		VkFormat _offscreenColorFormat{ VK_FORMAT_R32G32B32A32_SFLOAT };
		VkFormat _offscreenDepthFormat{ VK_FORMAT_X8_D24_UNORM_PACK32 };

		// post
		PostPipeline _postPipeline;
		VkRenderPass _postRenderPass;
		VkFormat _colorFormat{ VK_FORMAT_B8G8R8A8_UNORM };
		VkFormat _depthFormat{ VK_FORMAT_UNDEFINED };
		nvvk::DescriptorSetBindings _postDescSetLayoutBind;
		VkDescriptorPool _postDescPool{ VK_NULL_HANDLE };
		VkDescriptorSetLayout _postDescSetLayout{ VK_NULL_HANDLE };
		VkDescriptorSet _postDescSet{ VK_NULL_HANDLE };

		nvvk::SwapChain _swapchain;
		std::vector<VkFramebuffer> _framebuffers;
		std::vector<CommandList> _commandLists;
		std::vector<VkFence> _fences;
		VkImage _depthImage;
		VkDeviceMemory _depthMemory;
		VkImageView _depthView;
		VkExtent2D _size{ 0,0 };

		App* _app;
		glm::vec4 _clearColor;
		int _maxDepth{ 10 };

		void renderRaster(const CommandList& cmdBuf, Scene* scene, RasterPipeline* pipeline, const std::vector<VkDescriptorSet>& descSets);
		void renderRaytrace(const CommandList& cmdBuf, SceneRT* scene, RayTracingPipeline* pipeline, const std::vector<VkDescriptorSet>& descSets);
		void beginPost();
		void endPost(const CommandList& cmdBuf);
		void renderPost(const CommandList& cmdBuf); // mandatory step! after drawing

		void setViewport(const CommandList& cmdBuf);


		void prepareFrame();
		void createSwapchain(const VkSurfaceKHR& surface, uint32_t width, uint32_t height, VkFormat colorFormat = VK_FORMAT_B8G8R8A8_UNORM, VkFormat depthFormat = VK_FORMAT_UNDEFINED);
		void requestSwapchainImage(int w, int h);
		void createOffscreenRender(nvvk::ResourceAllocatorDma& alloc);
		void createPostDescriptorSet();
		void updatePostDescriptorSet();
		void createFrameBuffers();
		void createDepthBuffer();
		void createRenderPass();
		void createPostPipeline();

	public:
		void init(App* app, nvvk::ResourceAllocatorDma& alloc);
		void linkApp(App* app);
		void destroy(nvvk::ResourceAllocator* alloc);
		Astra::CommandList beginFrame();
		// renders both offscreen and post 
		void render(const CommandList& cmdBuf, Scene* scene, Pipeline* pipeline, const std::vector<VkDescriptorSet>& descSets, Astra::GuiController* gui = nullptr);
		void endFrame(const CommandList& cmdBuf);
		void resize(int w, int h, nvvk::ResourceAllocatorDma& alloc);

		glm::vec4& getClearColorRef();
		glm::vec4 getClearColor() const;
		void setClearColor(const glm::vec4& color);
		int& getMaxDepthRef();
		int getMaxDepth() const;
		void setMaxDepth(int depth);


		const nvvk::Texture& getOffscreenColor() const;
		VkRenderPass getOffscreenRenderPass() const;

		void getGuiControllerInfo(VkRenderPass& renderpass, int& imageCount, VkFormat& colorFormat, VkFormat& depthFormat);

	};
}