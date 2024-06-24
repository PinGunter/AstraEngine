#include "AppRT.h"

void Astra::AppRT::init(const std::vector<Scene*>& scenes, Renderer* renderer, GuiController* gui)
{
	Astra::App::init(scenes, renderer, gui);
	createRtDescriptorSetLayout();
	updateRtDescriptorSet();
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
	VkAccelerationStructureKHR tlas = ((DefaultSceneRT*)_scenes[_currentScene])->getTLAS();
	VkWriteDescriptorSetAccelerationStructureKHR descASInfo{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR };
	descASInfo.accelerationStructureCount = 1;
	descASInfo.pAccelerationStructures = &tlas;
	VkDescriptorImageInfo imageInfo{ {}, _renderer->getOffscreenColor().descriptor.imageView, VK_IMAGE_LAYOUT_GENERAL };

	std::vector<VkWriteDescriptorSet> writes;
	writes.emplace_back(_rtDescSetLayoutBind.makeWrite(_rtDescSet, RtxBindings::eTlas, &descASInfo));
	writes.emplace_back(_rtDescSetLayoutBind.makeWrite(_rtDescSet, RtxBindings::eOutImage, &imageInfo));
	vkUpdateDescriptorSets(AstraDevice.getVkDevice(), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);

}