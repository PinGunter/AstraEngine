#pragma once
#include <vulkan/vulkan.h>
#include <imgui.h>
#include <imgui/ImGuizmo.h>
#include <Renderer.h>
#include <GLFW/glfw3.h>
#include <App.h>

namespace Astra {
	class App;
	class GuiController {
		friend class Renderer;
	protected:
		VkDescriptorPool _imguiDescPool;
	public:
		void init(GLFWwindow* window, Renderer* renderer);
		void startFrame();
		void endFrame(const VkCommandBuffer& cmdBuf);
		void destroy();
		virtual void draw(App* app) {};
	};

	class BasiGui : public GuiController {
		bool _showGuizmo{ false };
		int _node{ 0 };
		int _ncopies{ 1 };
		bool _handlingNodes{ true };
	public:
		void draw(App* app) override;
	};
}