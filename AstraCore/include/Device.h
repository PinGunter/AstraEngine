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

namespace Astra
{

	/**
	 * @struct DeviceCreateInfo
	 * \~spanish @brief Contiene los parámetros necesarios para configurar el dispositivo (GPU). Por defecto utiliza ray-tracing \n
	 * Las extensiones se configuran por defecto con las necesarias para mostrar resultados por la pantalla y par el ray-tracing
	 * @warning
	 * Si no se rellenan los datos de capas de instancia y extensiones de instancia o dispositivo se asume que el usuario entiende lo que está haciendo.
	 * Esto quiere decir que el propio usuario @e experto deberá agregar las extensiones de GLFW y el resto de extensiones básicas como las de la swapchain.
	 *
	 * \~english @brief Contains the parameters to set up the device (GPU). Ray-Tracing is the default. Extensions are set up by default, extensions include the swapchain, present and ray-tracing ones.
	 * @warning
	 * If the instanceLayers, instanceExtensions or deviceExtensions vectors are empty the user is assumed to be an @e expert. This means that this user would also need
	 * to add all the extensions needed that are added by default. This means the GLFW extensions, the swapchain extensions, etc.
	 */
	struct DeviceCreateInfo
	{
		bool debug{true};
		bool useRT{true};

		std::vector<std::string> instanceLayers;
		std::vector<std::string> instanceExtensions;
		std::vector<std::string> deviceExtensions;
		uint32_t vkVersionMajor{1};
		uint32_t vkVersionMinor{3};
	};
	/**
	 * @class Device
	 * \~spanish @brief Singleton. Contiene los datos de más bajo nivel de Vulkan. Proporciona métodos para inicializar, crear shaders, texturas, UBOs, etc.
	 * \~english @brief Singleton. Contains the low-level Vulkan objects. Has methods for initializating itself, for creating shader modules, textures, UBOs, etc.
	 */
	class Device
	{
	private:
		VkInstance _instance;
		VkDevice _vkdevice;
		VkSurfaceKHR _surface;
		GLFWwindow *_window;
		VkPhysicalDevice _physicalDevice;
		VkQueue _queue;
		uint32_t _graphicsQueueIndex;
		VkCommandPool _cmdPool;
		bool _raytracingEnabled;

		nvvk::DebugUtil _debug;
		nvvk::Context _vkcontext{};

		VkPhysicalDeviceRayTracingPipelinePropertiesKHR _rtProperties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};

		Device() {}
		~Device()
		{
			glfwDestroyWindow(_window);
		}

	public:
		static Device &getInstance()
		{
			static Device instance;
			return instance;
		}

		Device(const Device &) = delete;
		Device &operator=(const Device &) = delete;
		/**
		 * \~spanish @brief Inicializa Vulkan y sus objetos de bajo nivel. También añade las extensiones indicadas por el createInfo.
		 * \~english @brief Contains the low-level Vulkan objects. Has methods for initializating itself, for creating shader modules, textures, UBOs, etc. Adds the extensions listed by the createInfo object.
		 */
		void initDevice(DeviceCreateInfo createInfo);
		void destroy();

		VkInstance getVkInstance() const;
		VkDevice getVkDevice() const;
		VkSurfaceKHR getSurface() const;
		VkPhysicalDevice getPhysicalDevice() const;
		VkQueue getQueue() const;
		uint32_t getGraphicsQueueIndex() const;
		VkCommandPool getCommandPool() const;
		GLFWwindow *getWindow();
		bool getRtEnabled() const;
		VkPhysicalDeviceRayTracingPipelinePropertiesKHR getRtProperties() const;

		/**
		 * \~spanish @brief Crear un shader module con el archivo binario recibido como parámetro
		 * \~english @brief Creates a shader module object with the binary file passed as an argument
		 */
		VkShaderModule createShaderModule(const std::vector<char> &file);
		VkShaderModule createShaderModule(const std::string &file);
		/**
		 * \~spanish @brief Añade al vector de textures @p textures las nuevas texturas indicadas por el vector @p new_textures .
		 * \~english @brief Adds to the @p textures vector the textured included in the @p new_textures vector.
		 */
		void createTextureImages(const Astra::CommandList &cmdList, const std::vector<std::string> &new_textures, std::vector<nvvk::Texture> &textures, nvvk::ResourceAllocatorDma &alloc);
		/**
		 * \~spanish @brief Convierte un Mesh a un objeto apropiado para la construcción de estructuras de aceleración.
		 * \~english @brief Transforms the Mesh object into an appropiate format for building the acceleration structure
		 */
		nvvk::RaytracingBuilderKHR::BlasInput objectToVkGeometry(const Astra::Mesh &model);

		/**
		 * \~spanish @brief Crea un UBO del tipo @p T
		 * @warning El tipo que se utilice para crear un UBO deberá estar declarado en el fichero @file host_device.h o alguno que incluyan el resto de shaders.
		 * \~english @brief Creates an UBO of type @p T
		 * @warning The type used for the UBO has to be declared in the @file host_device.h file or some file else that every shader includes.
		 */
		template <typename T>
		nvvk::Buffer createUBO(nvvk::ResourceAllocator *alloc);
		/**
		 * \~spanish @brief Actualiza el UBO @p deviceBuffer con los datos de @p hostUBO
		 * \~english @brief Updates an UBO, @p deviceBuffer with the data inside @p hostUBO
		 */
		template <typename T>
		void updateUBO(T hostUBO, nvvk::Buffer &deviceBuffer, const CommandList &cmdList);

		uint32_t getMemoryType(uint32_t typeBits, const VkMemoryPropertyFlags &properties) const;
		std::array<int, 2> getWindowSize() const;

		/**
		 * \~spanish @brief Espera a que la GPU este libre
		 * \~english @brief Waits for the GPU to be idle
		 */
		void waitIdle();
		/**
		 * \~spanish @brief Espera a que la cola de la GPU esté libre
		 * \~english @brief Waits for the GPU queue to be idle.
		 */
		void queueWaitIdle();
	};

#define AstraDevice Astra::Device::getInstance()

	template <typename T>
	inline nvvk::Buffer Device::createUBO(nvvk::ResourceAllocator *alloc)
	{
		return alloc->createBuffer(sizeof(T), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
								   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	}

	template <typename T>
	inline void Device::updateUBO(T hostUBO, nvvk::Buffer &deviceBuffer, const CommandList &cmdList)
	{

		// UBO on the device, and what stages access it.
		VkBuffer deviceUBO = deviceBuffer.buffer;
		uint32_t uboUsageStages = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
		if (getRtEnabled())
			uboUsageStages = uboUsageStages | VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;

		// Ensure that the modified UBO is not visible to previous frames.
		VkBufferMemoryBarrier beforeBarrier{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
		beforeBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		beforeBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		beforeBarrier.buffer = deviceUBO;
		beforeBarrier.offset = 0;
		beforeBarrier.size = sizeof(hostUBO);
		cmdList.pipelineBarrier(uboUsageStages, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_DEVICE_GROUP_BIT, {}, {beforeBarrier}, {});

		// Schedule the host-to-device upload. (hostUBO is copied into the cmd
		// buffer so it is okay to deallocate when the function returns).
		cmdList.updateBuffer(deviceBuffer, 0, sizeof(T), &hostUBO);

		// Making sure the updated UBO will be visible.
		VkBufferMemoryBarrier afterBarrier{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
		afterBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		afterBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		afterBarrier.buffer = deviceUBO;
		afterBarrier.offset = 0;
		afterBarrier.size = sizeof(hostUBO);
		cmdList.pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, uboUsageStages, VK_DEPENDENCY_DEVICE_GROUP_BIT, {}, {afterBarrier}, {});
	}
}