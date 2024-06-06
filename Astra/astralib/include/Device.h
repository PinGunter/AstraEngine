#pragma once
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <string>
#include <Singleton.h>
#include <nvvk/resourceallocator_vk.hpp>
#include <nvvk/debug_util_vk.hpp>

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

	class Device{
	private:
		VkInstance _instance;
		VkDevice _vkdevice;
		VkSurfaceKHR _surface;
		GLFWwindow* _window;
		VkPhysicalDevice _physicalDevice;
		VkQueue _queue;
		uint32_t _graphicsQueueIndex;
		VkCommandPool _cmdPool;

		nvvk::ResourceAllocatorDma _alloc; 
		nvvk::DebugUtil            _debug;

		Device() {}
		~Device() {
			// quizas encargarse de eliminar recursos
			// o en app quizas
		}
	public:

		static Device& getInstance() {
			static Device instance;
			return instance;
		}

		Device(const Device&) = delete;
		Device& operator=(const Device&) = delete;

		VkInstance getVkInstance() const;
		VkDevice getVkDevice() const;
		VkSurfaceKHR getSurface() const;
		VkPhysicalDevice getPhysicalDevice() const;
		VkQueue getQueue() const;
		uint32_t getGraphicsQueueIndex() const;
		VkCommandPool getCommandPool() const;
		GLFWwindow* getWindow();

		void initDevice(DeviceCreateInfo createInfo);

		VkCommandBuffer createCmdBuf();
		VkCommandBuffer createTmpCmdBuf();
		void submitTmbCmdBuf(VkCommandBuffer cmdBuff);

		VkShaderModule createShaderModule(const std::vector<char>& file);
	};
}