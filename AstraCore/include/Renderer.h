#pragma once
#include <Pipeline.h>
#include <Scene.h>
#include <App.h>
#include <nvvk/swapchain_vk.hpp>
#include <GLFW/glfw3.h>
#include <CommandList.h>
#include <GuiController.h>

namespace Astra
{
	class App;
	class GuiController;

	// the default renderer uses an offscreen renderpass to a texture
	// then renders that texture as a fullscreen triangle
	// this allows us to mix raytracing and rasterization
	// main use case is raytracing the scene and then rasterizing the ui
	/**
	 * \~spanish @brief Clase renderer. Contiene todo lo necesario para renderizar la escena. Se encarga de todo el proceso de dibujado.
	 * Contiene también los descriptor sets y pipeline de post procesado. Es una clase que interactua con vulkan a bajo nivel y no está pensada para ser sobreescrita. \n
	 * La forma en la que funciona el renderer es con dos render pass. El primero rasteriza o traza con rayos la escena y guarda el resultado en una textura.
	 * Esa textura luego se utiliza en el segundo render pass para mostrarse en pantalla junto con la interfaz de ImGui. De esta forma podemos combinar el trazado de rayos
	 * para modelos de la escena con la rasterización para fuentes y elementos gráficos de la interfaz.
	 * \~english @brief Renderer class. Contains all the neccessary info to render the scene, low-level kind. It manages the rendering process.
	 * Also contains the post processing descriptor sets and pipeline. This is an advance class and it's not meant to be overriden.
	 * The way the renderer works is with two render passes. The first rasterizes or ray traces the scene and saves the result in a texture.
	 * That texture is then used in the second rendering pass to be displayed on the screen along with the ImGui interface.
	 * This way we can combine both ray-tracing (for the scene) and rasterization (for fonts and graphical elements).
	 */
	class Renderer
	{
	protected:
		// offscreen
		VkRenderPass _offscreenRenderPass;
		VkFramebuffer _offscreenFb;
		nvvk::Texture _offscreenColor;
		nvvk::Texture _offscreenDepth;
		VkFormat _offscreenColorFormat{VK_FORMAT_R32G32B32A32_SFLOAT};
		VkFormat _offscreenDepthFormat{VK_FORMAT_X8_D24_UNORM_PACK32};

		// post
		PostPipeline _postPipeline;
		VkRenderPass _postRenderPass;
		VkFormat _colorFormat{VK_FORMAT_B8G8R8A8_UNORM};
		VkFormat _depthFormat{VK_FORMAT_UNDEFINED};
		nvvk::DescriptorSetBindings _postDescSetLayoutBind;
		VkDescriptorPool _postDescPool{VK_NULL_HANDLE};
		VkDescriptorSetLayout _postDescSetLayout{VK_NULL_HANDLE};
		VkDescriptorSet _postDescSet{VK_NULL_HANDLE};

		nvvk::SwapChain _swapchain;
		std::vector<VkFramebuffer> _framebuffers;
		std::vector<CommandList> _commandLists;
		std::vector<VkFence> _fences;
		VkImage _depthImage;
		VkDeviceMemory _depthMemory;
		VkImageView _depthView;
		VkExtent2D _size{0, 0};

		App *_app;
		glm::vec4 _clearColor;
		int _maxDepth{10};

		void renderRaster(const CommandList &cmdBuf, Scene *scene, RasterPipeline *pipeline, const std::vector<VkDescriptorSet> &descSets);
		void renderRaytrace(const CommandList &cmdBuf, SceneRT *scene, RayTracingPipeline *pipeline, const std::vector<VkDescriptorSet> &descSets);
		void beginPost();
		void endPost(const CommandList &cmdBuf);
		void renderPost(const CommandList &cmdBuf); // mandatory step! after drawing

		void setViewport(const CommandList &cmdBuf);

		void prepareFrame();
		void createSwapchain(const VkSurfaceKHR &surface, uint32_t width, uint32_t height, VkFormat colorFormat = VK_FORMAT_B8G8R8A8_UNORM, VkFormat depthFormat = VK_FORMAT_UNDEFINED);
		void requestSwapchainImage(int w, int h);
		void createOffscreenRender(nvvk::ResourceAllocatorDma &alloc);
		void createPostDescriptorSet();
		void updatePostDescriptorSet();
		void createFrameBuffers();
		void createDepthBuffer();
		void createRenderPass();
		void createPostPipeline();

	public:
		void init(App *app, nvvk::ResourceAllocatorDma &alloc);
		void linkApp(App *app);
		void destroy(nvvk::ResourceAllocator *alloc);
		/**
		 * \~spanish @brief Inicia el renderizado de la escena
		 * @return Un objeto commandList que sera necesario para que la escena actualice sus buffers.
		 * \~english @brief Begins the rendering of the scene
		 * @return A commandList object, needed by the scene to update their buffers.
		 */
		Astra::CommandList beginFrame();
		/**
		 * \~spanish @brief Renderiza la escena, raster o trazado de rayos según el tipo de pipeline que se le pase. También hace el dibujo de la interfaz y el postprocesado.
		 * @param cmdList el objeto commandList que devuelve el beginFrame();
		 * \~english @brief Renders the scene, either rasterized o raytraced. Also draws the gui and post processing effects
		 * @param cmdList the commandList object returned by the beginFrame() method.
		 */
		void render(const CommandList &cmdList, Scene *scene, Pipeline *pipeline, const std::vector<VkDescriptorSet> &descSets, Astra::GuiController *gui = nullptr);
		/**
		 * \~spanish @brief Finaliza el renderizado del frame
		 * @param cmdList el objeto commandList que devuelve el beginFrame();
		 * \~english @brief Ends the render of the frame
		 * @param cmdList the commandList object returned by the beginFrame() method.
		 */
		void endFrame(const CommandList &cmdList);
		void resize(int w, int h, nvvk::ResourceAllocatorDma &alloc);

		glm::vec4 &getClearColorRef();
		glm::vec4 getClearColor() const;
		void setClearColor(const glm::vec4 &color);
		int &getMaxDepthRef();
		int getMaxDepth() const;
		void setMaxDepth(int depth);

		const nvvk::Texture &getOffscreenColor() const;
		VkRenderPass getOffscreenRenderPass() const;

		/**
		 * \~spanish @brief Devuelve los atributos necesarios para configurar la interfaz de imgui. Modifica la referencia que recibe.
		 * \~english @brief Returns the necessary info for imgui. It modifies the object by reference.
		 */
		void getGuiControllerInfo(VkRenderPass &renderpass, int &imageCount, VkFormat &colorFormat, VkFormat &depthFormat);
	};
}