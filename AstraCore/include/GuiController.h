#pragma once
#include <vulkan/vulkan.h>
#include <imgui.h>
#include <ImGuizmo.h>
#include <Renderer.h>
#include <GLFW/glfw3.h>
#include <App.h>

namespace Astra
{
	class App;
	class Renderer;
	/**
	 * @class GuiController
	 * \~spanish @brief Clase sencilla que contiene métodos para dibujar una interfaz de ImGui
	 * \~english @brief Simple class to draw ImGui GUI over the app.
	 */
	class GuiController
	{
		friend class Renderer;

	protected:
		VkDescriptorPool _imguiDescPool;

	public:
		/**
		 * \~spanish @brief Inicializa el backend de ImGui
		 * \~english @brief Inits ImGui's backend
		 */
		void init(GLFWwindow *window, Renderer *renderer);
		/**
		 * \~spanish @brief Comienza el frame de la interfaz
		 * \~english @brief Begins the GUI frame
		 */
		void startFrame();
		/**
		 * \~spanish @brief Termina el frame de la interfaz
		 * \~english @brief Ends the GUI frame
		 */
		void endFrame(const CommandList &cmdList);
		/**
		 * \~spanish @brief Destruye los recursos usados
		 * \~english @brief Destroys resources
		 */
		void destroy();
		/**
		 * \~spanish @brief Método abstracto que necesita ser sobreescrito por las clases hijas. La implementación de esta función consiste en agregar las instrucciones de ImGui necesarias para dibujar la GUI.
		 * \~english @brief This virtual method has to be overridden by its derived classes. This method's implementation should include every ImGui instruccion for drawing a GUI.
		 */
		virtual void draw(App *app) = 0;
	};
}