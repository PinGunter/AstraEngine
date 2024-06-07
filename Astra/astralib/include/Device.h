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
		VkPhysicalDeviceRayTracingPipelinePropertiesKHR _rtProperties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR };
		bool _raytracingEnabled;

		nvvk::ResourceAllocatorDma _alloc; 
		nvvk::DebugUtil            _debug;
		nvvk::RaytracingBuilderKHR _rtBuilder;

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
		VkPhysicalDeviceRayTracingPipelinePropertiesKHR getRTProperties();
		nvvk::ResourceAllocatorDma& getResAlloc();
		bool getRtEnabled() const;


		VkCommandBuffer createCmdBuf();
		VkCommandBuffer createTmpCmdBuf();
		void submitTmpCmdBuf(VkCommandBuffer cmdBuff);

		VkShaderModule createShaderModule(const std::vector<char>& file);
		void createTextureImages(const VkCommandBuffer& cmdBuf, const std::vector<std::string>& new_textures, std::vector<nvvk::Texture>& textures);
		nvvk::RaytracingBuilderKHR::BlasInput objectToVkGeometry(const Astra::HostModel& model);
		

		uint32_t getMemoryType(uint32_t typeBits, const VkMemoryPropertyFlags& properties) const;
	};
}