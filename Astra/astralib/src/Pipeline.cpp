#include <Pipeline.h>
#include <nvvk/shaders_vk.hpp>
#include <Device.h>
#include <Utils.h>
#include <array>
#include <nvh/fileoperations.hpp>
#include <nvpsystem.hpp>
#include <host_device.h>

void Astra::Pipeline::bind(const VkCommandBuffer& cmdBuf, const std::vector<VkDescriptorSet>& descsets)
{
	vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);
	vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, _layout, 0,
		static_cast<uint32_t>(descsets.size()), descsets.data(),
		0, nullptr);
}

void Astra::Pipeline::pushConstants(const VkCommandBuffer& cmdBuf, uint32_t shaderStages, uint32_t size, void* data)
{
	vkCmdPushConstants(cmdBuf, _layout, shaderStages, 0, size, data);
}

void Astra::Pipeline::destroy(VkDevice vkdev)
{
	vkDestroyPipelineLayout(vkdev, _layout, nullptr);
	vkDestroyPipeline(vkdev, _pipeline, nullptr);
}

VkPipeline Astra::Pipeline::getPipeline()
{
	return _pipeline;
}

VkPipelineLayout Astra::Pipeline::getLayout()
{
	return _layout;
}

void Astra::RayTracingPipeline::createPipeline(VkDevice vkdev, const std::vector<VkDescriptorSetLayout>& descsets)
{

	std::vector<std::string> defaultSearchPaths = {
		NVPSystem::exePath() + PROJECT_RELDIRECTORY,
		NVPSystem::exePath() + PROJECT_RELDIRECTORY "..",
		std::string(PROJECT_NAME),
	};

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
	stage.module = nvvk::createShaderModule(vkdev, nvh::loadFile("spv/raytrace.rmiss.spv", true, defaultSearchPaths, true));
	stage.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
	stages[eRaygen] = stage;

	// miss
	stage.module = nvvk::createShaderModule(vkdev, nvh::loadFile("spv/raytrace.rmiss.spv", true, defaultSearchPaths, true));
	stage.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
	stages[eMiss] = stage;

	// shadow miss
	stage.module = nvvk::createShaderModule(vkdev, nvh::loadFile("spv/raytraceShadow.rmiss.spv", true, defaultSearchPaths, true));
	stage.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
	stages[eMiss2] = stage;

	// chit
	stage.module = nvvk::createShaderModule(vkdev, nvh::loadFile("spv/raytrace.rchit.spv", true, defaultSearchPaths, true));
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
	_rtShaderGroups.push_back(group);

	// miss
	group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
	group.generalShader = eMiss;
	_rtShaderGroups.push_back(group);

	// shadow miss
	group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
	group.generalShader = eMiss2;
	_rtShaderGroups.push_back(group);

	// closest hit shader
	group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
	group.generalShader = VK_SHADER_UNUSED_KHR;
	group.closestHitShader = eClosestHit;
	_rtShaderGroups.push_back(group);

	// push constants
	VkPushConstantRange pushConstant{ VK_SHADER_STAGE_RAYGEN_BIT_KHR |
		VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_NV, 0, sizeof(PushConstantRay) };

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstant;

	// descriptor sets: one specific to rt (set=0, tlas) , other shared with raster (set=1, scene data)
	pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t> (descsets.size());
	pipelineLayoutCreateInfo.pSetLayouts = descsets.data();

	if ((vkCreatePipelineLayout(vkdev, &pipelineLayoutCreateInfo, nullptr, &_layout) != VK_SUCCESS)) {
		throw std::runtime_error("Error creating pipeline layout");
	};

	// assemble the stage shaders and recursion depth
	VkRayTracingPipelineCreateInfoKHR rayPipelineInfo{ VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR };
	rayPipelineInfo.stageCount = static_cast<uint32_t>(stages.size());
	rayPipelineInfo.pStages = stages.data();

	// we indicate the shader groups
	// miss and raygen are their own group each
	// intersection, anyhit and chit form a hit group
	rayPipelineInfo.groupCount = static_cast<uint32_t>(_rtShaderGroups.size());
	rayPipelineInfo.pGroups = _rtShaderGroups.data();

	// recursion depth
	rayPipelineInfo.maxPipelineRayRecursionDepth = 2; // shadow
	rayPipelineInfo.layout = _layout;

	if ((vkCreateRayTracingPipelinesKHR(vkdev, {}, {}, 1, &rayPipelineInfo, nullptr, &_pipeline) != VK_SUCCESS) ){
		throw std::runtime_error("Error creating pipelines");
	}

	for (auto& s : stages) {
		vkDestroyShaderModule(vkdev, s.module, nullptr);
	}
}

void Astra::OffscreenRaster::createPipeline(VkDevice vkdev, const std::vector<VkDescriptorSetLayout>& descsetsLayouts, VkRenderPass rp)
{
	std::vector<std::string> defaultSearchPaths = {
		NVPSystem::exePath() + PROJECT_RELDIRECTORY,
		NVPSystem::exePath() + PROJECT_RELDIRECTORY "..",
		std::string(PROJECT_NAME),
	};

	VkPushConstantRange pushConstantRanges = { VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstantRaster) };

	// Creating the Pipeline Layout
	VkPipelineLayoutCreateInfo createInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	createInfo.setLayoutCount = 1;
	createInfo.pSetLayouts = descsetsLayouts.data();
	createInfo.pushConstantRangeCount = 1;
	createInfo.pPushConstantRanges = &pushConstantRanges;
	vkCreatePipelineLayout(vkdev, &createInfo, nullptr, &_layout);


	// Creating the Pipeline
	std::vector<std::string>                paths = defaultSearchPaths;
	nvvk::GraphicsPipelineGeneratorCombined gpb(vkdev, _layout, rp);
	gpb.depthStencilState.depthTestEnable = true;
	gpb.addShader(nvh::loadFile("spv/vert_shader.vert.spv", true, paths, true), VK_SHADER_STAGE_VERTEX_BIT);
	gpb.addShader(nvh::loadFile("spv/frag_shader.frag.spv", true, paths, true), VK_SHADER_STAGE_FRAGMENT_BIT);
	gpb.addBindingDescription({ 0, sizeof(Vertex) });
	gpb.addAttributeDescriptions({
		{0, 0, VK_FORMAT_R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(Vertex, pos))},
		{1, 0, VK_FORMAT_R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(Vertex, nrm))},
		{2, 0, VK_FORMAT_R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(Vertex, color))},
		{3, 0, VK_FORMAT_R32G32_SFLOAT, static_cast<uint32_t>(offsetof(Vertex, texCoord))},
		});

	_pipeline = gpb.createPipeline();
}

void Astra::PostPipeline::createPipeline(VkDevice vkdev, const std::vector<VkDescriptorSetLayout>& descsetsLayouts, VkRenderPass rp)
{
	std::vector<std::string> defaultSearchPaths = {
		NVPSystem::exePath() + PROJECT_RELDIRECTORY,
		NVPSystem::exePath() + PROJECT_RELDIRECTORY "..",
		std::string(PROJECT_NAME),
	};

	// Push constants in the fragment shader
	VkPushConstantRange pushConstantRanges = { VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float) };

	// Creating the pipeline layout
	VkPipelineLayoutCreateInfo createInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	createInfo.setLayoutCount = 1;
	createInfo.pSetLayouts = descsetsLayouts.data();
	createInfo.pushConstantRangeCount = 1;
	createInfo.pPushConstantRanges = &pushConstantRanges;
	vkCreatePipelineLayout(vkdev, &createInfo, nullptr, &_layout);


	// Pipeline: completely generic, no vertices
	nvvk::GraphicsPipelineGeneratorCombined pipelineGenerator(vkdev, _layout, rp);
	pipelineGenerator.addShader(nvh::loadFile("spv/passthrough.vert.spv", true, defaultSearchPaths, true), VK_SHADER_STAGE_VERTEX_BIT);
	pipelineGenerator.addShader(nvh::loadFile("spv/post.frag.spv", true, defaultSearchPaths, true), VK_SHADER_STAGE_FRAGMENT_BIT);
	pipelineGenerator.rasterizationState.cullMode = VK_CULL_MODE_NONE;
	_pipeline = pipelineGenerator.createPipeline();
}
