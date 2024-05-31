#pragma once
#include <vulkan/vulkan.h>
#include <vector>
namespace Astra {

	struct PushConstantCreateInfo {
		VkPushConstantRange range;
		uint32_t count;
		uint32_t size;
	};

	class Pipeline {
	private:
		VkPipelineLayout _layout;
		VkPipeline _pipeline;
		PushConstantCreateInfo _pushConsts;
		std::vector<VkDescriptorSetLayout> _descSetLayouts;
		std::vector<VkDescriptorSet> _descSets;

		void createLayout(const PushConstantCreateInfo& pushConstants, const std::vector<DescriptorSets>& descriptorSets);
		virtual void createPipeline() = 0; // raytracing/raster pipelines will define their creation
	public:
		inline virtual bool doesRayTracing() = 0;
		void bind(const VkCommandBuffer& cmdBuf);
		void pushConstants(const VkCommandBuffer& cmdBuf, uint32_t shaderStages, uint32_t size, void* data);
	};

	class RasterPipeline : public Pipeline {
	private:
		void createPipeline() override;
	public:
		inline bool doesRayTracing() override;
	};

	class RayTracingPipeline : public Pipeline {
	private:
		void createPipeline() override;
	public:
		inline bool doesRayTracing() override;
	};
}