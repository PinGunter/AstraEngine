#include "AppRT.h"

void Astra::AppRT::init(const std::vector<Scene*>& scenes, Renderer* renderer, GuiController* gui)
{
	if (!AstraDevice.getRtEnabled())
	{
		throw std::runtime_error("Raytracing has to be enabled!");
	}
	Astra::App::init(scenes, renderer, gui);
	createRtDescriptorSetLayout();
	updateRtDescriptorSet();
}

void Astra::AppRT::destroy()
{
	if (_status == Astra::Running)
	{
		App::destroy();
		vkDestroyDescriptorSetLayout(AstraDevice.getVkDevice(), _rtDescSetLayout, nullptr);
		vkDestroyDescriptorPool(AstraDevice.getVkDevice(), _rtDescPool, nullptr);
	}
}

void Astra::AppRT::createRtDescriptorSetLayout()
{
	const auto& device = AstraDevice.getVkDevice();
	_rtDescSetLayoutBind.addBinding(RtxBindings::eTlas, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
	_rtDescSetLayoutBind.addBinding(RtxBindings::eOutImage, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR);

	_rtDescPool = _rtDescSetLayoutBind.createPool(device);
	_rtDescSetLayout = _rtDescSetLayoutBind.createLayout(device);

	VkDescriptorSetAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	allocateInfo.descriptorPool = _rtDescPool;
	allocateInfo.descriptorSetCount = 1;
	allocateInfo.pSetLayouts = &_rtDescSetLayout;
	vkAllocateDescriptorSets(device, &allocateInfo, &_rtDescSet);
}

void Astra::AppRT::updateRtDescriptorSet()
{
	VkAccelerationStructureKHR tlas = ((SceneRT*)_scenes[_currentScene])->getTLAS();
	VkWriteDescriptorSetAccelerationStructureKHR descASInfo{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR };
	descASInfo.accelerationStructureCount = 1;
	descASInfo.pAccelerationStructures = &tlas;
	VkDescriptorImageInfo imageInfo{ {}, _renderer->getOffscreenColor().descriptor.imageView, VK_IMAGE_LAYOUT_GENERAL };

	std::vector<VkWriteDescriptorSet> writes;
	writes.emplace_back(_rtDescSetLayoutBind.makeWrite(_rtDescSet, RtxBindings::eTlas, &descASInfo));
	writes.emplace_back(_rtDescSetLayoutBind.makeWrite(_rtDescSet, RtxBindings::eOutImage, &imageInfo));
	vkUpdateDescriptorSets(AstraDevice.getVkDevice(), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}

void Astra::AppRT::onResize(int w, int h)
{
	Astra::App::onResize(w, h);
	updateRtDescriptorSet();
}

void Astra::AppRT::resetScene(bool recreatePipelines)
{
	_scenes[_currentScene]->reset();
	_descSetLayoutBind.clear();
	_rtDescSetLayoutBind.clear();
	createDescriptorSetLayout();
	updateDescriptorSet();
	createRtDescriptorSetLayout();
	updateRtDescriptorSet();

	if (recreatePipelines)
	{
		destroyPipelines();
		createPipelines();
	}

}
