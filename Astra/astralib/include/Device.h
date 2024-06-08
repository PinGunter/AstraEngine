#pragma once
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <string>
#include <nvvk/resourceallocator_vk.hpp>
#include <nvvk/debug_util_vk.hpp>
#include <nvvk/raytraceKHR_vk.hpp>
#include <Mesh.h>

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
		bool _raytracingEnabled;

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

		void initDevice(DeviceCreateInfo createInfo);
		void destroy();

		VkInstance getVkInstance() const;
		VkDevice getVkDevice() const;
		VkSurfaceKHR getSurface() const;
		VkPhysicalDevice getPhysicalDevice() const;
		VkQueue getQueue() const;
		uint32_t getGraphicsQueueIndex() const;
		VkCommandPool getCommandPool() const;
		GLFWwindow* getWindow();
		bool getRtEnabled() const;


		VkCommandBuffer createCmdBuf();
		VkCommandBuffer createTmpCmdBuf();
		void submitTmpCmdBuf(VkCommandBuffer cmdBuff);

		VkShaderModule createShaderModule(const std::vector<char>& file);
		void Device::createTextureImages(const VkCommandBuffer& cmdBuf, const std::vector<std::string>& new_textures, std::vector<nvvk::Texture>& textures, nvvk::ResourceAllocatorDma& alloc);		
		nvvk::RaytracingBuilderKHR::BlasInput objectToVkGeometry(const Astra::HostModel& model);

		uint32_t getMemoryType(uint32_t typeBits, const VkMemoryPropertyFlags& properties) const;
		std::array<int, 2> getWindowSize() const;
	};

#define AstraDevice Astra::Device::getInstance()
}