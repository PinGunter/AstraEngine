#pragma once
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include <string>
#include <App.h>

namespace Astra
{
	// forward declaration
	class App;

	/**
	 * @class InputManager
	 * \~spanish @brief Singleton. Clase que maneja todos los eventos del usuario y provee acceso global a estos. Llama a los métodos que gestionan eventos de la aplicación.
	 * \~english @brief Singleton. Class that manages all user input and provides global access to the rest of the library. Calls the event handling methods of the app.
	 */
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
		static InputManager& getInstance()
		{
			static InputManager instance;
			return instance;
		}
		InputManager(const InputManager&) = delete;
		InputManager& operator=(const InputManager&) = delete;

		/**
		 * \~spanish @brief Inicializa los callbacks
		 * \~english @brief Inits and sets up the callbacks
		 */
		void init(GLFWwindow* window, App* app);
		/**
		 * \~spanish @brief Consulta los eventos de GLFW
		 * @warning Este método se debe llamar al inicio de cada iteración del bucle de dibujado (método run()) de la aplicación!
		 * \~english @brief Polls the GLFW events
		 * @warning This method should be called every single iteration of the draw loop of the app (in run()) !
		 */
		void pollEvents();

		/**
		 * \~spanish @brief Oculta el cursor del ratón
		 * \~english @brief Hides the mouse cursor
		 */
		void hideMouse() const;
		/**
		 * \~spanish @brief Esconde y captura el cursor del ratón
		 * \~english @brief Hides and captures the mouse cursor.
		 */
		void captureMouse() const;
		/**
		 * \~spanish @brief Libera el ratón a su estado original
		 * \~english @brief Frees the mouse to its original state.
		 */
		void freeMouse() const;

		/**
		 * \~spanish @brief Devuelve la posición del ratón
		 * \~english @brief Returns the mouse position
		 */
		glm::ivec2 getMousePos() const;
		/**
		 * \~spanish @brief Devuelve la diferencia entre la posición actual y la anterior del ratón.
		 * \~english @brief Returns the difference between the current and past mouse positions.
		 */
		glm::ivec2 getMouseDelta() const;
		/**
		 * \~spanish @brief Devuelve el estado de la rueda del ratón.
		 * \~english @brief Returns the mouse wheel data.
		 */
		glm::ivec2 getMouseWheel() const;
		/**
		 * \~spanish @brief Consulta el estado del click del mouse.
		 * @retval true si esta pulsado ese botón en ese momento
		 * @retval false en caso contrario.
		 * \~english @brief Checks the mouse button state.
		 * @retval true if it's clicked
		 * @retval false if it's not.
		 */
		bool mouseClick(MouseButtons button) const;
		/**
		 * \~spanish @brief Devuelve el estado de una tecla que se le pasa como parámetro.
		 * @retval true  si está pulsada
		 * @retval false si no está pulsada
		 * \~english @brief Checks the keyboard key press state.
		 * @retval true  if the key is pressed
		 * @retval false if the key isn't pressed
		 */
		bool keyPressed(Keys key) const;

		/**
		 * \~spanish @brief Devuelve si la ventana se debería cerrar o no.
		 * \~english @brief Returns whether the window should close or not.
		 */
		bool windowShouldClose() const;

		/**
		 * \~spanish @brief Establece el estado de si la ventana debería cerrar o no.
		 * \~english @brief Stablishes the window should close variable
		 */
		void setWindowShouldClose(bool s);
	};

#define Input InputManager::getInstance()

}