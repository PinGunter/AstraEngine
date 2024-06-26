#pragma once
#include <nvvk/resourceallocator_vk.hpp>
#include <GuiController.h>
#include <Renderer.h>
#include <Device.h>
#include <Scene.h>
#include <CommandList.h>
#include <InputManager.h>
namespace Astra {
	class GuiController;
	class Renderer;
	class Scene;

	class App {
		friend class Renderer;
		friend class InputManager;
	protected:
		AppStatus _status{ Created };
		std::vector<Scene*> _scenes;
		int _currentScene{ 0 };
		GuiController* _gui;
		Renderer* _renderer;
		GLFWwindow* _window;

		nvvk::ResourceAllocatorDma _alloc;

		std::vector<Pipeline*> _pipelines;
		int _selectedPipeline{ 0 };

		// descriptor sets (common for both raster and raytracing apps)
		nvvk::DescriptorSetBindings _descSetLayoutBind;
		VkDescriptorPool _descPool;
		VkDescriptorSetLayout _descSetLayout;
		VkDescriptorSet _descSet;

		virtual void createPipelines() = 0;
		virtual void destroyPipelines();
		virtual void createDescriptorSetLayout();
		virtual void updateDescriptorSet();

		virtual void resetScene(bool recreatePipelines = false) = 0;


		virtual void onResize(int w, int h);
		virtual void onFileDrop(int count, const char** paths) {};
		virtual void onMouseMotion(int x, int y) {};
		virtual void onKeyboard(int key, int scancode, int action, int mods) {};
		virtual void onMouseButton(int button, int action, int mods) {};
		virtual void onMouseWheel(int x, int y) {};
	public:
		virtual void init(const std::vector<Scene*>& scenes, Renderer* renderer, GuiController* gui = nullptr);
		virtual void addScene(Scene* s);
		virtual void run() = 0;
		~App();

		virtual void destroy();

		bool isMinimized() const;
		int& getCurrentSceneIndexRef();
		int getCurrentSceneIndex() const;
		virtual void setCurrentSceneIndex(int i);

		Scene* getCurrentScene();
		Renderer* getRenderer();

		AppStatus getStatus() const;


	};
}