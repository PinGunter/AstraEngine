#pragma once
#include <nvvk/resourceallocator_vk.hpp>
#include <GuiController.h>
#include <Renderer.h>
#include <Device.h>
#include <Scene.h>
namespace Astra {
	class GuiController;
	class Renderer;
	class Scene;

	class App {	
		friend class Renderer;
	protected:
		const std::vector<Scene*> _scenes;
		int _currentScene{ 0 };
		GuiController* _gui;
		Renderer* _renderer;
		GLFWwindow* _window;

		nvvk::Buffer _globalsBuffer; // UBO for camera 

		virtual void createUBO();
		virtual void updateUBO(const VkCommandBuffer& cmdBuf);
		virtual void createDescriptorSetLayout() {};
		virtual void updateDescriptorSet() {};
		virtual void createObjDescBuffer();

		virtual void onResize(int w, int h);
		virtual void onMouseMotion(int x, int y) {};
		virtual void onKeyboard(int key, int scancode, int action, int mods) {};
		virtual void onKeyboardChar(unsigned char key) {};
		virtual void onMouseButton(int button, int action, int mods) {};
		virtual void onMouseWheel(int x, int y) {};
		virtual void onFileDrop(int count, const char** paths) {};

		static void cb_resize(GLFWwindow* window, int w, int h);
		static void cb_keyboard(GLFWwindow* window, int key, int scancode, int action, int mods);
		static void cb_keyboardChar(GLFWwindow* window, unsigned int key);
		static void cb_mouseMotion(GLFWwindow* window, double x, double y);
		static void cb_mouseButton(GLFWwindow* window, int button, int action, int mods);
		static void cb_mouseWheel(GLFWwindow* window, double x, double y);
		static void cb_fileDrop(GLFWwindow* window, int count, const char** paths);


	public:
		void init();
		virtual void run() {};

		void destroy();
		nvvk::Buffer & getCameraUBO();

		void setupCallbacks(GLFWwindow * window);
		bool isMinimized() const;
		void loadModel(const std::string& filename, const glm::mat4& transform);
		int& getCurrentSceneIndexRef();
		int getCurrenSceneIndex() const;
		void setCurrentSceneIndex(int i);
	};

	class DefaultApp : public App {
	protected:
		// raster
		nvvk::DescriptorSetBindings _descSetLayoutBind;
		VkDescriptorPool _descPool;
		VkDescriptorSetLayout _descSetLayout;
		VkDescriptorSet _descSet;
		OffscreenRaster _rasterPipeline;
		
		// rt
		nvvk::DescriptorSetBindings _rtDescSetLayoutBind;
		VkDescriptorPool _rtDescPool;
		VkDescriptorSetLayout _rtDescSetLayout;
		VkDescriptorSet _rtDescSet;
		RayTracingPipeline _rtPipeline;
		std::vector<nvvk::AccelKHR> _blas;
		std::vector<VkAccelerationStructureInstanceKHR> m_tlas;

	public:
		void createDescriptorSetLayout() override;
		void updateDescriptorSet() override;
		void createRtDescriptorSet();
		void udpateRtDescriptorSet();

		void onResize(int w, int h) override;
		void onMouseMotion(int x, int y) override;
		void onMouseButton(int button, int action, int mods) override;
		void onMouseWheel(int x, int y) override;
	};
}