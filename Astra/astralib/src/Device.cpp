#include <Device.h>
#include <nvvk/context_vk.hpp>
#include <stdexcept>
#include <nvvk/images_vk.hpp>
#include <nvpsystem.hpp>
#include <nvh/fileoperations.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <nvvk/buffers_vk.hpp>

constexpr auto SAMPLE_WIDTH = 1280;
constexpr auto SAMPLE_HEIGHT = 720;

namespace Astra {

	// if the struct has any non-empty array we assume that it is and advanced
	// user and will provide every extensions or instance layer needed

	void Device::initDevice(DeviceCreateInfo createInfo) {
		bool emptyRT = false; // checks if the users wants raytracing and used empty device extensions. So that we can add them

		// create glfw window
		if (!glfwInit()) {
			throw std::runtime_error("Error initializing glfw");
		}
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		_window = glfwCreateWindow(SAMPLE_WIDTH, SAMPLE_HEIGHT, "Astra App", nullptr, nullptr);


		// fill createInfo with default values if empty
		if (createInfo.instanceLayers.empty()) {
			if (createInfo.debug) {
				createInfo.instanceLayers.push_back("VK_LAYER_KHRONOS_validation");
				createInfo.instanceLayers.push_back("VK_LAYER_LUNARG_monitor");
			}
		}

		if (createInfo.instanceExtensions.empty()) {
			uint32_t count{ 0 };
			auto glfwExtensions = glfwGetRequiredInstanceExtensions(&count);

			createInfo.instanceExtensions.resize(count);

			for (uint32_t i = 0; i < count; i++) {
				createInfo.instanceExtensions[i] = glfwExtensions[i];
			}

			if (createInfo.debug) {
				createInfo.instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			}
		}

		if (createInfo.deviceExtensions.empty()) {
			createInfo.deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
			createInfo.deviceExtensions.push_back(VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME); // enable shader printf

			emptyRT = createInfo.useRT;
		}



		nvvk::ContextCreateInfo contextInfo;
		contextInfo.setVersion(createInfo.vkVersionMajor, createInfo.vkVersionMinor);
		for (const auto& layer : createInfo.instanceLayers) {
			contextInfo.addInstanceLayer(layer.data(), true);
		}
		for (const auto& ext : createInfo.instanceExtensions) {
			contextInfo.addInstanceExtension(ext.data(), true);
		}
		for (const auto& ext : createInfo.deviceExtensions) {
			contextInfo.addDeviceExtension(ext.data(), true);
		}

		if (emptyRT) {
			VkPhysicalDeviceAccelerationStructureFeaturesKHR accelFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR };
			contextInfo.addDeviceExtension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, false, &accelFeatures); // to build acceleration structures
			VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtPipelineFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR };
			contextInfo.addDeviceExtension(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, false, &rtPipelineFeatures); // raytracing pipeline
			contextInfo.addDeviceExtension(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME); // required by raytracing pipeline
		}

		if (!_vkcontext.initInstance(contextInfo)) {
			throw std::runtime_error("Error creating instance!");
		}

		auto compatibleDevices = _vkcontext.getCompatibleDevices(contextInfo);
		if (compatibleDevices.empty()) {
			throw std::runtime_error("No valid GPU was found");
		}

		if (!_vkcontext.initDevice(compatibleDevices[0], contextInfo)) {
			throw std::runtime_error("Error initiating device!");
		}

		// filling the data
		_instance = _vkcontext.m_instance;
		_vkdevice = _vkcontext.m_device;
		_physicalDevice = _vkcontext.m_physicalDevice;
		_graphicsQueueIndex = _vkcontext.m_queueGCT.familyIndex;
		vkGetDeviceQueue(_vkdevice, _graphicsQueueIndex, 0, &_queue);

		// getting the surface

		VkSurfaceKHR surface{};
		if (glfwCreateWindowSurface(_instance, _window, nullptr, &surface) != VK_SUCCESS) {
			throw std::runtime_error("Error creating window surface");
		}
		_surface = surface;

		// will throw exception if not supported
		VkBool32 supported;
		vkGetPhysicalDeviceSurfaceSupportKHR(_physicalDevice, _graphicsQueueIndex, _surface, &supported);
		if (supported != VK_TRUE) {
			throw std::runtime_error("Error, device does not support presenting");
		}

		VkCommandPoolCreateInfo poolCreateInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		vkCreateCommandPool(_vkdevice, &poolCreateInfo, nullptr, &_cmdPool);

		// if no error was thrown it means that it supports RT
		_raytracingEnabled = createInfo.useRT;

		_debug.setup(_vkdevice);
	}

	VkInstance Device::getVkInstance() const
	{
		return _instance;
	}

	VkDevice Device::getVkDevice() const
	{
		return _vkdevice;
	}
	VkSurfaceKHR Device::getSurface() const
	{
		return _surface;
	}

	VkPhysicalDevice Device::getPhysicalDevice() const
	{
		return _physicalDevice;
	}

	VkQueue Device::getQueue() const
	{
		return _queue;
	}

	uint32_t Device::getGraphicsQueueIndex() const
	{
		return _graphicsQueueIndex;
	}
	VkCommandPool Device::getCommandPool() const
	{
		return _cmdPool;
	}

	GLFWwindow* Device::getWindow()
	{
		return _window;
	}

	VkCommandBuffer Device::createTmpCmdBuf()
	{
		// allocate 
		VkCommandBufferAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		allocateInfo.commandBufferCount = 1;
		allocateInfo.commandPool = _cmdPool;
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		// create
		VkCommandBuffer cmdBuffer;
		vkAllocateCommandBuffers(_vkdevice, &allocateInfo, &cmdBuffer);

		// start
		VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(cmdBuffer, &beginInfo);
		return cmdBuffer;
	}


	bool Device::getRtEnabled() const
	{
		return _raytracingEnabled;
	}

	void Device::submitTmpCmdBuf(VkCommandBuffer cmdBuff)
	{
		// end
		vkEndCommandBuffer(cmdBuff);
		// submit
		VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmdBuff;
		vkQueueSubmit(_queue, 1, &submitInfo, {});
		// sync
		vkQueueWaitIdle(_queue);
		// free
		vkFreeCommandBuffers(_vkdevice, _cmdPool, 1, &cmdBuff);
	}


	VkShaderModule Device::createShaderModule(const std::vector<char>& code)
	{
		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.flags = {};
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule = VK_NULL_HANDLE;
		if (vkCreateShaderModule(_vkdevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
		{
			throw std::runtime_error("Error creating shader module");
		}

		return shaderModule;
	}

	void Device::destroy()
	{
		if (_surface)
			vkDestroySurfaceKHR(_instance, _surface, nullptr);
	}

	uint32_t Device::getMemoryType(uint32_t typeBits, const VkMemoryPropertyFlags& properties) const
	{
		VkPhysicalDeviceMemoryProperties memoryProperties;
		vkGetPhysicalDeviceMemoryProperties(_physicalDevice, &memoryProperties);

		for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
		{
			if (((typeBits & (1 << i)) > 0) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
				return i;
		}
		throw std::runtime_error("Unable to find memory type");
	}

	std::array<int, 2> Device::getWindowSize() const
	{
		int w, h;
		glfwGetFramebufferSize(_window, &w, &h);
		return { w, h };
	}

	nvvk::RaytracingBuilderKHR::BlasInput Device::objectToVkGeometry(const Astra::HostModel& model)
	{
		// BLAS builder requires raw device addresses.
		VkDeviceAddress vertexAddress = nvvk::getBufferDeviceAddress(_vkdevice, model.vertexBuffer.buffer);
		VkDeviceAddress indexAddress = nvvk::getBufferDeviceAddress(_vkdevice, model.indexBuffer.buffer);

		uint32_t maxPrimitiveCount = model.nbIndices / 3;

		// Describe buffer as array of VertexObj.
		VkAccelerationStructureGeometryTrianglesDataKHR triangles{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR };
		triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;  // vec3 vertex position data.
		triangles.vertexData.deviceAddress = vertexAddress;
		triangles.vertexStride = sizeof(Vertex);
		// Describe index data (32-bit unsigned int)
		triangles.indexType = VK_INDEX_TYPE_UINT32;
		triangles.indexData.deviceAddress = indexAddress;
		// Indicate identity transform by setting transformData to null device pointer.
		//triangles.transformData = {};
		triangles.maxVertex = model.nbVertices - 1;

		// Identify the above data as containing opaque triangles.
		VkAccelerationStructureGeometryKHR asGeom{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
		asGeom.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		asGeom.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
		asGeom.geometry.triangles = triangles;

		// The entire array will be used to build the BLAS.
		VkAccelerationStructureBuildRangeInfoKHR offset;
		offset.firstVertex = 0;
		offset.primitiveCount = maxPrimitiveCount;
		offset.primitiveOffset = 0;
		offset.transformOffset = 0;

		// Our blas is made from only one geometry, but could be made of many geometries
		nvvk::RaytracingBuilderKHR::BlasInput input;
		input.asGeometry.emplace_back(asGeom);
		input.asBuildOffsetInfo.emplace_back(offset);

		return input;
	}

	void Device::createTextureImages(const VkCommandBuffer& cmdBuf, const std::vector<std::string>& new_textures, std::vector<nvvk::Texture>& textures, nvvk::ResourceAllocatorDma& alloc)
	{
		VkSamplerCreateInfo samplerCreateInfo{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
		samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerCreateInfo.maxLod = FLT_MAX;

		VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;

		// If no textures are present, create a dummy one to accommodate the pipeline layout
		if (new_textures.empty() && textures.empty())
		{
			nvvk::Texture texture;

			std::array<uint8_t, 4> color{ 255u, 255u, 255u, 255u };
			VkDeviceSize           bufferSize = sizeof(color);
			auto                   imgSize = VkExtent2D{ 1, 1 };
			auto                   imageCreateInfo = nvvk::makeImage2DCreateInfo(imgSize, format);

			// Creating the dummy texture
			nvvk::Image           image = alloc.createImage(cmdBuf, bufferSize, color.data(), imageCreateInfo);
			VkImageViewCreateInfo ivInfo = nvvk::makeImageViewCreateInfo(image.image, imageCreateInfo);
			texture = alloc.createTexture(image, ivInfo, samplerCreateInfo);

			// The image format must be in VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			nvvk::cmdBarrierImageLayout(cmdBuf, texture.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			textures.push_back(texture);
		}
		else
		{
			// Uploading all images
			for (const auto& texture : new_textures)
			{
				std::vector<std::string> defaultSearchPaths = {
					NVPSystem::exePath() + PROJECT_RELDIRECTORY,
					NVPSystem::exePath() + PROJECT_RELDIRECTORY "..",
					std::string(PROJECT_NAME),
				};

				std::stringstream o;
				int               texWidth, texHeight, texChannels;
				o << "media/textures/" << texture;
				std::string txtFile = nvh::findFile(o.str(), defaultSearchPaths, true);

				stbi_uc* stbi_pixels = stbi_load(txtFile.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

				std::array<stbi_uc, 4> color{ 255u, 0u, 255u, 255u };

				stbi_uc* pixels = stbi_pixels;
				// Handle failure
				if (!stbi_pixels)
				{
					texWidth = texHeight = 1;
					texChannels = 4;
					pixels = reinterpret_cast<stbi_uc*>(color.data());
				}

				VkDeviceSize bufferSize = static_cast<uint64_t>(texWidth) * texHeight * sizeof(uint8_t) * 4;
				auto         imgSize = VkExtent2D{ (uint32_t)texWidth, (uint32_t)texHeight };
				auto         imageCreateInfo = nvvk::makeImage2DCreateInfo(imgSize, format, VK_IMAGE_USAGE_SAMPLED_BIT, true);

				{
					nvvk::Image image = alloc.createImage(cmdBuf, bufferSize, pixels, imageCreateInfo);
					nvvk::cmdGenerateMipmaps(cmdBuf, image.image, format, imgSize, imageCreateInfo.mipLevels);
					VkImageViewCreateInfo ivInfo = nvvk::makeImageViewCreateInfo(image.image, imageCreateInfo);
					nvvk::Texture         texture = alloc.createTexture(image, ivInfo, samplerCreateInfo);

					textures.push_back(texture);
				}

				stbi_image_free(stbi_pixels);
			}
		}
	}


}