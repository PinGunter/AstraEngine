#include <Pipeline.h>

void Astra::Pipeline::bind(const VkCommandBuffer& cmdBuf)
{
	vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);
	vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, _layout, 0,
		static_cast<uint32_t>(_descSets.size()), _descSets.data(),
		0, nullptr);
}

void Astra::Pipeline::pushConstants(const VkCommandBuffer& cmdBuf, uint32_t shaderStages, uint32_t size, void* data)
{
	vkCmdPushConstants(cmdBuf, _layout, shaderStages, 0, size, data);
}

void Astra::RasterPipeline::createPipeline()
{
}

inline bool Astra::RasterPipeline::doesRayTracing()
{
	return false;
}

void Astra::RayTracingPipeline::createPipeline()
{
	enum StageIndices {
		eRaygen,
		eMiss,
		eMiss2,
		eClosestHit,
		eShaderGroupCount
	};

	// all stages
	std::array<VkPipelineShaderStageCreateInfo, eShaderGroupCount> stages{};
	VkPipelineShaderStageCreateInfo stage{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	stage.pName = "main";

	// raygen
	stage.module = nvvk::createShaderModule(m_device, nvh::loadFile("spv/raytrace.rgen.spv", true, defaultSearchPaths, true));
	stage.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
	stages[eRaygen] = stage;

	// miss
	stage.module = nvvk::createShaderModule(m_device, nvh::loadFile("spv/raytrace.rmiss.spv", true, defaultSearchPaths, true));
	stage.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
	stages[eMiss] = stage;

	// shadow miss
	stage.module = nvvk::createShaderModule(m_device, nvh::loadFile("spv/raytraceShadow.rmiss.spv", true, defaultSearchPaths, true));
	stage.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
	stages[eMiss2] = stage;

	// chit
	stage.module = nvvk::createShaderModule(m_device, nvh::loadFile("spv/raytrace.rchit.spv", true, defaultSearchPaths, true));
	stage.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
	stages[eClosestHit] = stage;

	// shader groups
	VkRayTracingShaderGroupCreateInfoKHR group{ VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR };
	group.anyHitShader = VK_SHADER_UNUSED_KHR;
	group.closestHitShader = VK_SHADER_UNUSED_KHR;
	group.generalShader = VK_SHADER_UNUSED_KHR;
	group.intersectionShader = VK_SHADER_UNUSED_KHR;

	// raygen
	group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
	group.generalShader = eRaygen;
	m_rtShaderGroups.push_back(group);

	// miss
	group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
	group.generalShader = eMiss;
	m_rtShaderGroups.push_back(group);

	// shadow miss
	group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
	group.generalShader = eMiss2;
	m_rtShaderGroups.push_back(group);

	// closest hit shader
	group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
	group.generalShader = VK_SHADER_UNUSED_KHR;
	group.closestHitShader = eClosestHit;
	m_rtShaderGroups.push_back(group);

	// push constants
	VkPushConstantRange pushConstant{ VK_SHADER_STAGE_RAYGEN_BIT_KHR |
		VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_NV, 0, sizeof(PushConstantRay) };

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstant;

	// descriptor sets: one specific to rt (set=0, tlas) , other shared with raster (set=1, scene data)
	std::vector<VkDescriptorSetLayout> rtDescSetLayouts = { m_rtDescSetLayout, m_descSetLayout };
	pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t> (rtDescSetLayouts.size());
	pipelineLayoutCreateInfo.pSetLayouts = rtDescSetLayouts.data();

	vkCreatePipelineLayout(m_device, &pipelineLayoutCreateInfo, nullptr, &m_rtPipelineLayout);

	// assemble the stage shaders and recursion depth
	VkRayTracingPipelineCreateInfoKHR rayPipelineInfo{ VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR };
	rayPipelineInfo.stageCount = static_cast<uint32_t>(stages.size());
	rayPipelineInfo.pStages = stages.data();

	// we indicate the shader groups
	// miss and raygen are their own group each
	// intersection, anyhit and chit form a hit group
	rayPipelineInfo.groupCount = static_cast<uint32_t>(m_rtShaderGroups.size());
	rayPipelineInfo.pGroups = m_rtShaderGroups.data();

	// recursion depth
	rayPipelineInfo.maxPipelineRayRecursionDepth = 2; // shadow
	rayPipelineInfo.layout = m_rtPipelineLayout;

	vkCreateRayTracingPipelinesKHR(m_device, {}, {}, 1, &rayPipelineInfo, nullptr, &m_rtPipeline);

	for (auto& s : stages) {
		vkDestroyShaderModule(m_device, s.module, nullptr);
	}
}

inline bool Astra::RayTracingPipeline::doesRayTracing()
{
	return true;
}
