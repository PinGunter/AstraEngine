#pragma once
#include <nvvk/resourceallocator_vk.hpp>
#include <GuiController.h>
#include <Renderer.h>
#include <Device.h>
#include <Scene.h>
#include <CommandList.h>
#include <InputManager.h>

namespace Astra
{
	class GuiController;
	class Renderer;
	class Scene;

	/**
	 * @class App App.h
	 *  \~spanish @brief Clase base para todas las aplicaciones que utilicen la biblioteca
	 *  \~english @brief Base class for every Astra application
	 */
	class App
	{
		friend class Renderer;
		friend class InputManager;

	protected:
		/**
		 *  \~spanish @brief Estado de la aplicación, puede ser creada, en ejecución y destruida
		 *  \~english @brief Application state, can be created, running or destroyed
		 */
		AppStatus _status{Created};
		/**
		 *  \~spanish @brief Vector de escenas
		 *  \~english @brief Scene vector
		 */
		std::vector<Scene *> _scenes;
		int _currentScene{0};

		GuiController *_gui;
		Renderer *_renderer;
		GLFWwindow *_window;

		/**
		 *  \~spanish @brief El asignador de resursos de la biblioteca nvpro_core. Se utiliza en algunos métodos para asignar y liberar memoria.
		 *  \~english @brief Nvidia Nvpro_core resource allocator. Needed for allocating and freeing memory and other objects.
		 */
		nvvk::ResourceAllocatorDma _alloc;

		/**
		 *  \~spanish @brief Vector de pipelines
		 *  \~english @brief Pipelines vector
		 */
		std::vector<Pipeline *> _pipelines;
		int _selectedPipeline{0};

		nvvk::DescriptorSetBindings _descSetLayoutBind;
		VkDescriptorPool _descPool;
		VkDescriptorSetLayout _descSetLayout;
		VkDescriptorSet _descSet;

		/**
		 *  \~spanish @brief Método abstracto que cada clase derivada debe implementar. Debe crear las pipelines que se vayan a usar e introducir en el vector @a _pipelines
		 *  \~english @brief Abstract method that has to be implemented by every derived class. Should create the pipelines needed for the app and push them in the @a _pipelines object.
		 */
		virtual void createPipelines() = 0;
		/**
		 *  \~spanish @brief Destruye las pipelines dentro del vector de pipelines
		 *  \~english @brief Destroys the pipelines within the @a _pipelines vector
		 */
		virtual void destroyPipelines();
		/**
		 *  \~spanish @brief Crea el layout de los descriptor sets básicos para rasterización.
		 *  					El descriptor set que crea tiene la siguiente estructura: \n
		 *  					Binding 0: Parámetros de Cámara \n
		 *  					Binding 1: Datos de las luces \n
		 *  					Binding 2: Datos de los objetos, direcciones de memoria para acceder a los buffers \n
		 *  					Binding 3: Texturas
		 *
		 *  \~english @brief Creates the descriptor set layouts needed for rasterization
		 *  					The bindings are the following: \n
		 * 						Binding 0: Camera parameters \n
		 * 						Binding 1: Data for the different lights in the scene \n
		 * 						Binding 2: Object descriptors, GPU addresses for the buffers \n
		 * 						Binding 3: Textures
		 */
		virtual void createDescriptorSetLayout();
		/**
		 *  \~spanish @brief Escribe y actualiza los datos del descriptor set
		 *  \~english @brief Writes and updates the descriptor set data
		 */
		virtual void updateDescriptorSet();
		/**
		 *  \~spanish @brief Resetea la escena, se debe llamar cuando se cambie de escena o si se agregan modelos durante la ejecución.
		 *  \~spanish @warning No se puede llamar durante el renderizado ya que los descriptor sets estarán desactualizados al mismo tiempo que se ejecutan los shaders. Asegurarse de hacerlo antes o después!.
		 *  \~english @brief Resets the scene. Should be called whenever the current scene changes or if models are added in runtime
		 *  \~english @warning Calling the method while rendering will probably crash the scene as Descriptor Sets will be outdated as the shaders are running! Make sure to reset it before or after a render action.
		 */
		virtual void resetScene(bool recreatePipelines = false) = 0;
		/**
		 *  \~spanish @brief Callback para el cambio de tamaño de ventana. Actualiza ImGui, renderer, cámara y descriptor sets
		 *  \~english @brief Window resize callback. Updates ImGui, renderer, camera and descriptor sets
		 */
		virtual void onResize(int w, int h);
		/**
		 *  \~spanish @brief Callback para cuando se suelten archivos en la ventana. No hace nada por defecto
		 *  \~english @brief File drop callback. Does nothing by default
		 */
		virtual void onFileDrop(int count, const char **paths) {};
		/**
		 *  \~spanish @brief Callback de evento de movimiento de ratón. No hace nada por defecto.
		 *  \~english @brief Mouse motion callback. Does nothing by default
		 */
		virtual void onMouseMotion(int x, int y) {};
		/**
		 *  \~spanish @brief Callback para cuando se pulsan teclas del teclado. No hace nada por defecto
		 *  \~english @brief Keyboard keys event callback. Does nothing by default
		 */
		virtual void onKeyboard(int key, int scancode, int action, int mods) {};
		/**
		 *  \~spanish @brief Callback para el click del ratón. No hace nada por defecto
		 *  \~english @brief Mouse click callback. Does nothing by default.
		 */
		virtual void onMouseButton(int button, int action, int mods) {};
		/**
		 *  \~spanish @brief Callback para el evento de la rueda del ratón. No hace nada por defecto.
		 *  \~english @brief Mouse wheel callback. Does nothing by default.
		 */
		virtual void onMouseWheel(int x, int y) {};

	public:
		/**
		 *  \~spanish @brief Inicializa la aplicación.
		 *  \~spanish @brief Inicializa al InputManager, las Escenas (Scene), el Renderer y la intefaz de usuario si hay (GuiController)
		 *  \~spanish @brief Tambien crea y escribe los descriptor sets
		 *  \~spanish @warning Se debe llamar en cada clase hija que se cree!
		 *  \~english @brief Initializes the app
		 *  \~english @brief Inits the InputManager, Scene, Renderer and GuiController objects
		 * 	\~english @brief Also creates and writes the descriptor sets
		 *  \~english @brief Has to be called inside every derived class' init method!
		 */
		virtual void init(const std::vector<Scene *> &scenes, Renderer *renderer, GuiController *gui = nullptr);
		/**
		 *  \~spanish @brief Añade una escena a la aplicación.
		 *  \~english @brief Adds a scene to the app
		 */
		virtual void addScene(Scene *s);
		/**
		 *  \~spanish @brief Método virtual que debe implementar la clase derivada. Se debe encargar de hacer el bucle de dibujado, hacer el polling de eventos (con InputManager), actualizar la escena y dibujarla. Después del bucle se deberían destruir los recursos \n
		 * Código de ejemplo:
		 * @code
		 * void run(){
		 * 	while (!Astra::Input.windowShouldClose()){
		 *		// no renderizar si está minimizada
		 *		if (isMinimized()) continue;
		 *
		 *		auto cmdList = _renderer->beginFrame();
		 *		updateScene(cmdList);
		 *		_renderer->render(cmdList, _scenes[_currentScene], _pipelines[_selectedPipeline], { _descSet }, _gui);
		 *		_renderer->endFrame();
		 *		}
		 * }
		 * destroy();
		 *  @endcode
		 *  \~english @brief Pure virtual method. The derived class should implement the drawing loop, the event polling (using InputManager), updating the scene and drawing it. After the loop, destroy() should be called \n
		 * Example code:
		 * @code
		 * void run(){
		 * 	while (!Astra::Input.windowShouldClose()){
		 *		// dont render if is minimized
		 *		if (isMinimized()) continue;
		 *
		 *		auto cmdList = _renderer->beginFrame();
		 *		updateScene(cmdList);
		 *		_renderer->render(cmdList, _scenes[_currentScene], _pipelines[_selectedPipeline], { _descSet }, _gui);
		 *		_renderer->endFrame();
		 *		}
		 * }
		 * destroy();
		 *  @endcode
		 */
		virtual void run() = 0;
		~App();

		virtual void destroy();
		bool isMinimized() const;
		int &getCurrentSceneIndexRef();
		int getCurrentSceneIndex() const;
		virtual void setCurrentSceneIndex(int i);

		Scene *getCurrentScene();
		Renderer *getRenderer();

		AppStatus getStatus() const;
	};
}