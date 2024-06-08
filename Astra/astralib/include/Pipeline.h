#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <nvvk/vulkanhppsupport.hpp>
namespace Astra {
	class Pipeline {
	protected:
		VkPipelineLayout _layout;
		VkPipeline _pipeline;

	public:
		~Pipeline();
		inline virtual bool doesRayTracing() = 0;
		virtual void bind(const VkCommandBuffer& cmdBuf, const std::vector<VkDescriptorSet> & descsets);
		void pushConstants(const VkCommandBuffer& cmdBuf, uint32_t shaderStages, uint32_t size, void* data);
		VkPipeline getPipeline();
		VkPipelineLayout getLayout();
	};

	class RasterPipeline : public Pipeline {
	protected:
	public:
		virtual void createPipeline(VkDevice vkdev, const std::vector<VkDescriptorSetLayout>& descsetsLayouts, VkRenderPass rp) = 0;
		inline bool doesRayTracing() override {
			return false;
		};
	};

	class RayTracingPipeline : public Pipeline {
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
		void bind(const VkCommandBuffer& cmdBuf, const std::vector<VkDescriptorSet>& descsets) override;
		void createPipeline(VkDevice vkdev, const std::vector<VkDescriptorSetLayout>& descsetsLayouts, nvvk::ResourceAllocatorDma& alloc, const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& rtProperties);
		inline bool doesRayTracing() override {
			return true;
		};
		std::array<VkStridedDeviceAddressRegionKHR, 4> getSBTRegions();
	};

	class OffscreenRaster : public RasterPipeline {
	public:
		void createPipeline(VkDevice vkdev, const std::vector<VkDescriptorSetLayout>& descsetsLayouts, VkRenderPass rp) override;
	};

	class PostPipeline : public RasterPipeline {
	public:
		void createPipeline(VkDevice vkdev, const std::vector<VkDescriptorSetLayout>& descsetsLayouts, VkRenderPass rp) override;
	};
}