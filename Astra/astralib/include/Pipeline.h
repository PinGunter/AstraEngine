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
		inline virtual bool doesRayTracing() = 0;
		virtual void bind(const VkCommandBuffer& cmdBuf, const std::vector<VkDescriptorSet> & descsets);
		void pushConstants(const VkCommandBuffer& cmdBuf, uint32_t shaderStages, uint32_t size, void* data);
		void destroy(VkDevice vkdev);
		VkPipeline getPipeline();
		VkPipelineLayout getLayout();
	};

	class RasterPipeline : public Pipeline {
	private:
	public:
		virtual void createPipeline(VkDevice vkdev, const std::vector<VkDescriptorSetLayout>& descsetsLayouts, VkRenderPass rp) = 0;
		inline bool doesRayTracing() override {
			return false;
		};
	};

	class RayTracingPipeline : public Pipeline {
	private:
		nvvk::DescriptorSetBindings _rtDescSetLayoutBind;
		std::vector<VkRayTracingShaderGroupCreateInfoKHR> _rtShaderGroups;
		

	public:
		void bind(const VkCommandBuffer& cmdBuf, const std::vector<VkDescriptorSet>& descsets) override;
		void createPipeline(VkDevice vkdev, const std::vector<VkDescriptorSetLayout>& descsetsLayouts);
		inline bool doesRayTracing() override {
			return true;
		};
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