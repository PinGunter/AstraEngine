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

	class DefaultApp : public App {
	protected:
		// raster
		nvvk::DescriptorSetBindings _descSetLayoutBind;
		VkDescriptorPool _descPool;
		VkDescriptorSetLayout _descSetLayout;
		VkDescriptorSet _descSet;
		Pipeline* _rasterPipeline;

		// rt
		nvvk::DescriptorSetBindings _rtDescSetLayoutBind;
		VkDescriptorPool _rtDescPool;
		VkDescriptorSetLayout _rtDescSetLayout;
		VkDescriptorSet _rtDescSet;
		Pipeline* _rtPipeline;
		std::vector<nvvk::AccelKHR> _blas;
		std::vector<VkAccelerationStructureInstanceKHR> m_tlas;
		VkPhysicalDeviceRayTracingPipelinePropertiesKHR _rtProperties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR };

		// wireframe pipeline;
		Pipeline* _wireframePipeline;
		glm::vec3 wireColor;

		int _selectedPipeline{ 0 };

		// models and instances to load after frame execution
		std::vector<MeshInstance> _newInstances;
		std::vector<std::pair<std::string, glm::mat4>> _newModels;
		bool _rendering = false;

		// camera and input controls
		bool _mouseButtons[3] = { 0 };
		int _lastMousePos[2] = { 0 };
		int _inputMods{ 0 };

		void createPipelines() override;
		void createDescriptorSetLayout() override;
		void updateDescriptorSet() override;
		void createRtDescriptorSet();
		void updateRtDescriptorSet();

		void onResize(int w, int h) override;
		void onMouseMotion(int x, int y) override;
		void onMouseButton(int button, int action, int mods) override;
		void onMouseWheel(int x, int y) override;
		void onKeyboard(int key, int scancode, int action, int mods) override;
		void onFileDrop(int count, const char** paths) override;

		void resetScene();

	public:
		void init(const std::vector<Scene*>& scenes, Renderer* renderer, GuiController* gui = nullptr) override;
		void run() override;
		void destroy() override;

		// add models / instances in runtime
		void addModelToScene(const std::string& filepath, const glm::mat4& transform = glm::mat4(1.0f));
		void addInstanceToScene(const Astra::MeshInstance& instance);

		void setCurrentSceneIndex(int i) override;

		int& getSelectedPipelineRef();
	};
}