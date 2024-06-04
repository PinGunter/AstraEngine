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
		virtual void createPipeline(VkDevice vkdev, const std::vector<VkDescriptorSetLayout> & descsetsLayouts) = 0; // raytracing/raster pipelines will define their creation
		inline virtual bool doesRayTracing() = 0;
		void bind(const VkCommandBuffer& cmdBuf, const std::vector<VkDescriptorSet> & descsets);
		void pushConstants(const VkCommandBuffer& cmdBuf, uint32_t shaderStages, uint32_t size, void* data);
		void destroy(VkDevice vkdev);
		VkPipeline getPipeline();
		VkPipelineLayout getLayout();
	};

	class RasterPipeline : public Pipeline {
	private:
	public:
		void createPipeline(VkDevice vkdev, const std::vector<VkDescriptorSetLayout>& descsetsLayouts) override {};
		void createPipeline(VkDevice vkdev, const std::vector<VkDescriptorSetLayout>& descsetsLayouts, VkRenderPass rp);
		inline bool doesRayTracing() override {
			return false;
		};
	};

	class RayTracingPipeline : public Pipeline {
	private:
		nvvk::DescriptorSetBindings _rtDescSetLayoutBind;
		std::vector<VkRayTracingShaderGroupCreateInfoKHR> _rtShaderGroups;
		

	public:
		void createPipeline(VkDevice vkdev, const std::vector<VkDescriptorSetLayout>& descsetsLayouts) override;
		inline bool doesRayTracing() override {
			return true;
		};
	};
}