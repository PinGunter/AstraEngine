#pragma once
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include <string>
#include <App.h>

namespace Astra
{
	// forward declaration
	class App;
	class InputManager
	{
		App* _appPtr;
		GLFWwindow* _window;
		glm::ivec2 _lastMousePos;
		glm::ivec2 _mouseDelta;
		glm::ivec2 _mouseWheel;
		bool _mouseButton[MouseButtons::SIZE_MOUSEBUTTONS];
		bool _keyMap[Keys::SIZE_KEYMAP];

		void setUpCallbacks();
		void onResize(int w, int h);
		void onKey(int key, int scancode, int action, int mods);
		void onMouseMotion(int x, int y);
		void onMouseButton(int button, int action, int mods);
		void onMouseWheel(double x, double y);
		void onFileDrop(int count, const char** paths);

		InputManager() {}
		~InputManager() {}


		static void cb_resize(GLFWwindow* window, int w, int h);
		static void cb_keyboard(GLFWwindow* window, int key, int scancode, int action, int mods);
		static void cb_mouseMotion(GLFWwindow* window, double x, double y);
		static void cb_mouseButton(GLFWwindow* window, int button, int action, int mods);
		static void cb_mouseWheel(GLFWwindow* window, double x, double y);
		static void cb_fileDrop(GLFWwindow* window, int count, const char** paths);

	public:
		static InputManager& getInstance() {
			static InputManager instance;
			return instance;
		}
		InputManager(const InputManager&) = delete;
		InputManager& operator=(const InputManager&) = delete;

		void init(GLFWwindow* window, App* app);
		void pollEvents();

		void hideMouse();
		void captureMouse();
		void freeMouse();

		glm::ivec2 getMousePos();
		glm::ivec2 getMouseDelta();
		glm::ivec2 getMouseWheel();
		bool mouseClick(MouseButtons button);
		bool keyPressed(Keys key);
	};

#define Input InputManager::getInstance()

}