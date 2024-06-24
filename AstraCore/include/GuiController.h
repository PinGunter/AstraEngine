#pragma once
#include <vulkan/vulkan.h>
#include <imgui.h>
#include <ImGuizmo.h>
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
		void endFrame(const CommandList& cmdList);
		void destroy();
		virtual void draw(App* app) {};
	};
}