#include <InputManager.h>

void Astra::InputManager::setUpCallbacks()
{
	glfwSetWindowUserPointer(_window, this);
	glfwSetKeyCallback(_window, &cb_keyboard);
	glfwSetCursorPosCallback(_window, &cb_mouseMotion);
	glfwSetMouseButtonCallback(_window, &cb_mouseButton);
	glfwSetScrollCallback(_window, &cb_mouseWheel);
	glfwSetFramebufferSizeCallback(_window, &cb_resize);
	glfwSetDropCallback(_window, &cb_fileDrop);
}

void Astra::InputManager::onResize(int w, int h)
{
	_appPtr->onResize(w, h);
}

void Astra::InputManager::onKey(int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		_keyMap[key] = true;
	}
	else if (action == GLFW_RELEASE)
	{
		_keyMap[key] = false;
	}
	_appPtr->onKeyboard(key, scancode, action, mods);
}

void Astra::InputManager::onMouseMotion(int x, int y)
{
	_mouseDelta = {x - _lastMousePos.x, y - _lastMousePos.y};
	_lastMousePos = {x, y};
	_appPtr->onMouseMotion(x, y);
}

void Astra::InputManager::onMouseButton(int button, int action, int mods)
{
	_mouseButton[button] = action == GLFW_PRESS;
	_appPtr->onMouseButton(button, action, mods);
}

void Astra::InputManager::onMouseWheel(double x, double y)
{
	_mouseWheel = {x, y};
	_appPtr->onMouseWheel(x, y);
}

void Astra::InputManager::onFileDrop(int count, const char **paths)
{
	_appPtr->onFileDrop(count, paths);
}

void Astra::InputManager::cb_resize(GLFWwindow *window, int w, int h)
{
	auto manager = static_cast<Astra::InputManager *>(glfwGetWindowUserPointer(window));
	manager->onResize(w, h);
}

void Astra::InputManager::cb_keyboard(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	auto manager = static_cast<Astra::InputManager *>(glfwGetWindowUserPointer(window));
	manager->onKey(key, scancode, action, mods);
}

void Astra::InputManager::cb_mouseMotion(GLFWwindow *window, double x, double y)
{
	auto manager = static_cast<Astra::InputManager *>(glfwGetWindowUserPointer(window));
	manager->onMouseMotion(x, y);
}

void Astra::InputManager::cb_mouseButton(GLFWwindow *window, int button, int action, int mods)
{
	auto manager = static_cast<Astra::InputManager *>(glfwGetWindowUserPointer(window));
	manager->onMouseButton(button, action, mods);
}

void Astra::InputManager::cb_mouseWheel(GLFWwindow *window, double x, double y)
{
	auto manager = static_cast<Astra::InputManager *>(glfwGetWindowUserPointer(window));
	manager->onMouseWheel(x, y);
}

void Astra::InputManager::cb_fileDrop(GLFWwindow *window, int count, const char **paths)
{
	auto manager = static_cast<Astra::InputManager *>(glfwGetWindowUserPointer(window));
	manager->onFileDrop(count, paths);
}

void Astra::InputManager::init(GLFWwindow *window, App *app)
{
	_window = window;
	_appPtr = app;
	setUpCallbacks();
}

void Astra::InputManager::pollEvents()
{
	_mouseWheel = glm::vec2(0);
	_mouseDelta = glm::vec2(0);
	glfwPollEvents();
}

void Astra::InputManager::hideMouse() const
{
	glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
}

void Astra::InputManager::captureMouse() const
{
	glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void Astra::InputManager::freeMouse() const
{
	glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

glm::ivec2 Astra::InputManager::getMousePos() const
{
	return _lastMousePos;
}

glm::ivec2 Astra::InputManager::getMouseDelta() const
{
	return _mouseDelta;
}

glm::ivec2 Astra::InputManager::getMouseWheel() const
{
	return _mouseWheel;
}

bool Astra::InputManager::mouseClick(MouseButtons button) const
{
	return _mouseButton[button];
}

bool Astra::InputManager::keyPressed(Keys key) const
{
	return _keyMap[key];
}

bool Astra::InputManager::windowShouldClose() const
{
	return glfwWindowShouldClose(_window);
}
