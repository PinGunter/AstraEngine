#pragma once
#include <nvvk/resourceallocator_vk.hpp>
#include <GUI.h>
#include <Renderer.h>
#include <Device.h>
namespace Astra {
	class App {	
		friend class Renderer;
	protected:
		//Astra::Gui* _gui;
		Astra::Renderer* _renderer;
		Astra::Scene _scene; // TODO change to vector ? 
		GLFWwindow* _window;

		nvvk::Buffer _globalsBuffer; // UBO for camera 

		virtual void updateUBO(const VkCommandBuffer& cmdBuf);
		virtual void createDescriptorSetLayout();
		virtual void updateDescriptorSet();
		virtual void createUBO();
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

		void destroy();
		nvvk::Buffer & getCameraUBO();

		void setupCallbacks(GLFWwindow * window);
		bool isMinimized() const;
		void loadModel(const std::string& filename, const glm::mat4& transform);
	};

	class DefaultApp : public App {
	protected:
		nvvk::DescriptorSetBindings _descSetLayoutBind;
		VkDescriptorPool            _descPool;
		VkDescriptorSetLayout       _descSetLayout;
		VkDescriptorSet             _descSet;
	public:
		void createDescriptorSetLayout() override;
		void updateDescriptorSet() override;

		void onResize(int w, int h) override;
		void onMouseMotion(int x, int y) override;
		void onKeyboard(int key, int scancode, int action, int mods) override;
		void onKeyboardChar(unsigned char key) override;
		void onMouseButton(int button, int action, int mods) override;
		void onMouseWheel(int x, int y) override;
		void onFileDrop(int count, const char** paths) override;
	};
}