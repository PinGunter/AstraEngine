#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <nvvk/vulkanhppsupport.hpp>
#include <CommandList.h>
namespace Astra {
	class CommandList;
	class Pipeline {
	protected:
		VkPipelineLayout _layout;
		VkPipeline _pipeline;

	public:
		virtual void destroy(nvvk::ResourceAllocator* alloc);
		inline virtual bool doesRayTracing() = 0;
		virtual void bind(const CommandList& cmdList, const std::vector<VkDescriptorSet>& descsets);
		virtual void pushConstants(const CommandList& cmdList, uint32_t shaderStages, uint32_t size, void* data);
		VkPipeline getPipeline() const;
		VkPipelineLayout getLayout() const;
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
		void bind(const CommandList& cmdList, const std::vector<VkDescriptorSet>& descsets) override;
		void createPipeline(VkDevice vkdev, const std::vector<VkDescriptorSetLayout>& descsetsLayouts, nvvk::ResourceAllocatorDma& alloc);
		inline bool doesRayTracing() override {
			return true;
		};
		std::array<VkStridedDeviceAddressRegionKHR, 4> getSBTRegions();
		void destroy(nvvk::ResourceAllocator* alloc) override;
	};

	class OffscreenRaster : public RasterPipeline {
	public:
		void createPipeline(VkDevice vkdev, const std::vector<VkDescriptorSetLayout>& descsetsLayouts, VkRenderPass rp) override;
	};

	class PostPipeline : public RasterPipeline {
	public:
		void createPipeline(VkDevice vkdev, const std::vector<VkDescriptorSetLayout>& descsetsLayouts, VkRenderPass rp) override;
	};

	class WireframePipeline : public RasterPipeline {
	public:
		void createPipeline(VkDevice vkdev, const std::vector<VkDescriptorSetLayout>& descsetsLayouts, VkRenderPass rp) override;
	};
}