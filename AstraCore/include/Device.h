#pragma once
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <string>
#include <nvvk/resourceallocator_vk.hpp>
#include <nvvk/debug_util_vk.hpp>
#include <nvvk/raytraceKHR_vk.hpp>
#include <Mesh.h>
#include <nvvk/context_vk.hpp>
#include <CommandList.h>

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
		VkDevice _vkdevice;
		VkSurfaceKHR _surface;
		GLFWwindow* _window;
		VkPhysicalDevice _physicalDevice;
		VkQueue _queue;
		uint32_t _graphicsQueueIndex;
		VkCommandPool _cmdPool;
		bool _raytracingEnabled;

		nvvk::DebugUtil            _debug;
		nvvk::Context _vkcontext{};

		VkPhysicalDeviceRayTracingPipelinePropertiesKHR _rtProperties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR };

		Device() {}
		~Device() {
			glfwDestroyWindow(_window);
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
		VkPhysicalDeviceRayTracingPipelinePropertiesKHR getRtProperties() const;

		VkShaderModule createShaderModule(const std::vector<char>& file);
		void createTextureImages(const Astra::CommandList& cmdList, const std::vector<std::string>& new_textures, std::vector<nvvk::Texture>& textures, nvvk::ResourceAllocatorDma& alloc);
		nvvk::RaytracingBuilderKHR::BlasInput objectToVkGeometry(const Astra::Mesh& model);

		template<typename T>
		nvvk::Buffer createUBO(nvvk::ResourceAllocator* alloc);
		template<typename T>
		void updateUBO(T hostUBO, nvvk::Buffer& deviceBuffer, const CommandList& cmdList);

		uint32_t getMemoryType(uint32_t typeBits, const VkMemoryPropertyFlags& properties) const;
		std::array<int, 2> getWindowSize() const;

		void waitIdle();
		void queueWaitIdle();
	};

#define AstraDevice Astra::Device::getInstance()


	template<typename T>
	inline nvvk::Buffer Device::createUBO(nvvk::ResourceAllocator* alloc)
	{
		return alloc->createBuffer(sizeof(T), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	}

	template<typename T>
	inline void Device::updateUBO(T hostUBO, nvvk::Buffer& deviceBuffer, const CommandList& cmdList) {

		// UBO on the device, and what stages access it.
		VkBuffer deviceUBO = deviceBuffer.buffer;
		uint32_t uboUsageStages = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
		if (getRtEnabled())
			uboUsageStages = uboUsageStages | VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;

		// Ensure that the modified UBO is not visible to previous frames.
		VkBufferMemoryBarrier beforeBarrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
		beforeBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		beforeBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		beforeBarrier.buffer = deviceUBO;
		beforeBarrier.offset = 0;
		beforeBarrier.size = sizeof(hostUBO);
		cmdList.pipelineBarrier(uboUsageStages, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_DEVICE_GROUP_BIT, {}, { beforeBarrier }, {});

		// Schedule the host-to-device upload. (hostUBO is copied into the cmd
		// buffer so it is okay to deallocate when the function returns).
		cmdList.updateBuffer(deviceBuffer, 0, sizeof(T), &hostUBO);

		// Making sure the updated UBO will be visible.
		VkBufferMemoryBarrier afterBarrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
		afterBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		afterBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		afterBarrier.buffer = deviceUBO;
		afterBarrier.offset = 0;
		afterBarrier.size = sizeof(hostUBO);
		cmdList.pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, uboUsageStages, VK_DEPENDENCY_DEVICE_GROUP_BIT, {}, { afterBarrier }, {});

	}
}