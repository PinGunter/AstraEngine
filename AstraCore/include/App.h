#pragma once
#include <nvvk/resourceallocator_vk.hpp>
#include <GuiController.h>
#include <Renderer.h>
#include <Device.h>
#include <Scene.h>
#include <CommandList.h>
namespace Astra {
	class GuiController;
	class Renderer;
	class Scene;

	class App {
		friend class Renderer;
	protected:
		AppStatus _status{ Created };
		std::vector<Scene*> _scenes;
		int _currentScene{ 0 };
		GuiController* _gui;
		Renderer* _renderer;
		GLFWwindow* _window;

		nvvk::Buffer _globalsBuffer; // UBO for camera 
		nvvk::ResourceAllocatorDma _alloc;

		std::vector<Pipeline*> _pipelines;

		virtual void createUBO();
		virtual void updateUBO(CommandList& cmdList);
		virtual void createDescriptorSetLayout() {};
		virtual void updateDescriptorSet() {};
		virtual void createPipelines() {};
		virtual void destroyPipelines();

		virtual void onResize(int w, int h) {};
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
		virtual void init(const std::vector<Scene*>& scenes, Renderer* renderer, GuiController* gui = nullptr);
		virtual void addScene(Scene* s);
		virtual void run();
		~App();

		virtual void destroy();
		nvvk::Buffer& getCameraUBO();

		void setupCallbacks(GLFWwindow* window);
		bool isMinimized() const;
		int& getCurrentSceneIndexRef();
		int getCurrentSceneIndex() const;
		virtual void setCurrentSceneIndex(int i);

		Scene* getCurrentScene();
		Renderer* getRenderer();

		AppStatus getStatus() const;
	};
}