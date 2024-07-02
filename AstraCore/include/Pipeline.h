#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <nvvk/descriptorsets_vk.hpp>
#include <CommandList.h>
namespace Astra
{
	class CommandList;
	/**
	 * \~spanish @brief Clase abstracta que representa una pipeline de Vulkan.
	 * \~english @brief Abstract class that represents a Vulkan pipeline
	 */
	class Pipeline
	{
	protected:
		VkPipelineLayout _layout;
		VkPipeline _pipeline;

	public:
		virtual void destroy(nvvk::ResourceAllocator* alloc);
		inline virtual bool doesRayTracing() = 0;
		/**
		 * \~spanish @brief Método para activar la pipeline durante el renderizado
		 * \~english @brief Binds the pipeline to the render pass
		 */
		virtual void bind(const CommandList& cmdList, const std::vector<VkDescriptorSet>& descsets);
		/**
		 * \~spanish @brief Sube las push constants a la GPU
		 * \~english @brief Pushes push constants to the GPU
		 */
		virtual void pushConstants(const CommandList& cmdList, uint32_t shaderStages, uint32_t size, void* data);
		VkPipeline getPipeline() const;
		VkPipelineLayout getLayout() const;
	};
	/**
	 * \~spanish @brief Clase abstracta para pipelines de rasterización. Declara el método de crear para ser sobreescrito por sus hijas.
	 * \~english @brief Abstract class for raster pipelines. Declares the create method for its derived class to override
	 */
	class RasterPipeline : public Pipeline
	{
	protected:
	public:
		/**
		 * \~spanish @brief Se tiene que sobreescribir. Provee la lista de parámetros y generalización.
		 * \~english @brief Has to be overriden. Provides the parameter list and generalization. (for upcasting)
		 */
		virtual void create(VkDevice vkdev, const std::vector<VkDescriptorSetLayout>& descsetsLayouts, VkRenderPass rp) = 0;
		inline bool doesRayTracing() override
		{
			return false;
		};
	};
	/**
	 * \~spanish @brief Clase de pipeline de trazado de rayos. Está completamente implementada para usarse
	 * \~english @brief RayTracing pipeline. Complete implementation. Ready for use.
	 */
	class RayTracingPipeline : public Pipeline
	{
	protected:
		VkStridedDeviceAddressRegionKHR _rgenRegion{};
		VkStridedDeviceAddressRegionKHR _missRegion{};
		VkStridedDeviceAddressRegionKHR _hitRegion{};
		VkStridedDeviceAddressRegionKHR _callRegion{};
		nvvk::Buffer _rtSBTBuffer;

		nvvk::DescriptorSetBindings _rtDescSetLayoutBind;
		std::vector<VkRayTracingShaderGroupCreateInfoKHR> _rtShaderGroups;
		void createSBT(nvvk::ResourceAllocatorDma& alloc, const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& rtProperties);

	public:
		void bind(const CommandList& cmdList, const std::vector<VkDescriptorSet>& descsets) override;
		virtual void create(VkDevice vkdev, const std::vector<VkDescriptorSetLayout>& descsetsLayouts, nvvk::ResourceAllocatorDma& alloc);
		inline bool doesRayTracing() override
		{
			return true;
		};
		std::array<VkStridedDeviceAddressRegionKHR, 4> getSBTRegions();
		void destroy(nvvk::ResourceAllocator* alloc) override;
	};
	/**
	 * \~spanish @brief Pipeline de rasterización de ejemplo para dibujar escenas en el offscreen framebuffer. Lista para usarse
	 * \~english @brief Offscreen raster pipeline. Ready to use.
	 */
	class OffscreenRaster : public RasterPipeline
	{
	public:
		void create(VkDevice vkdev, const std::vector<VkDescriptorSetLayout>& descsetsLayouts, VkRenderPass rp) override;
	};
	/**
	 * \~spanish @brief Pipeline que dibuja la textura resultante del offscreen render en pantalla. Aplica postprocesado también
	 * \~english @brief Postprocessing pipeline. Draws the offscreen texture.
	 */
	class PostPipeline : public RasterPipeline
	{
	public:
		void create(VkDevice vkdev, const std::vector<VkDescriptorSetLayout>& descsetsLayouts, VkRenderPass rp) override;
	};
}