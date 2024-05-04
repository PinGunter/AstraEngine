#pragma once
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <string>

namespace Astra {

	struct DeviceCreateInfo {
		bool debug{ true };
		bool useRT{ true };

		std::vector<std::string> instanceLayers;
		std::vector<std::string> instanceExtensions;
		std::vector<std::string> deviceExtensions;
		uint32_t vkVersionMajor{ 1 };
		uint32_t vkVersionMinor{ 3 };
	};

	class Device {
	private:
		VkInstance _instance;
		VkDevice _device;
		VkSurfaceKHR _surface;
		GLFWwindow* _window;
		VkPhysicalDevice _physicalDevice;
		VkQueue _queue;
		uint32_t _graphicsQueueIndex;
		VkCommandPool _cmdPool;

	public:
		Device(DeviceCreateInfo createInfo, GLFWwindow * window);
	};
}