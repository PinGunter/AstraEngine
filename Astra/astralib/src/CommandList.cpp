#include <CommandList.h>
#include <Device.h>

Astra::CommandList::CommandList(const VkCommandBuffer& cmdBuf) : _cmdBuf(cmdBuf) {}

VkCommandBuffer Astra::CommandList::getCommandBuffer()
{
	return _cmdBuf;
}

void Astra::CommandList::pipelineBarrier(VkPipelineStageFlags srcFlags, VkPipelineStageFlags dstFlags, VkDependencyFlags depsFlags, const std::vector<VkMemoryBarrier>& memoryBarrier, const std::vector<VkBufferMemoryBarrier>& bufferMemoryBarrier, const std::vector<VkImageMemoryBarrier>& imageMemoryBarrier)
{
	vkCmdPipelineBarrier(_cmdBuf, srcFlags, dstFlags, depsFlags, memoryBarrier.size(), memoryBarrier.data(), bufferMemoryBarrier.size(), bufferMemoryBarrier.data(), imageMemoryBarrier.size(), imageMemoryBarrier.data());
}

void Astra::CommandList::updateBuffer(const nvvk::Buffer& buffer, uint32_t offset, VkDeviceSize size, const void* data)
{
	vkCmdUpdateBuffer(_cmdBuf, buffer.buffer, offset, size, data);
}

void Astra::CommandList::submitTmpCmdList()
{
	// end
	vkEndCommandBuffer(_cmdBuf);
	// submit
	VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &_cmdBuf;
	vkQueueSubmit(AstraDevice.getQueue(), 1, &submitInfo, {});
	// sync
	vkQueueWaitIdle(AstraDevice.getQueue());
	// free
	vkFreeCommandBuffers(AstraDevice.getVkDevice(), AstraDevice.getCommandPool(), 1, &_cmdBuf);
}

Astra::CommandList Astra::CommandList::createTmpCmdList()
{
	// allocate
	VkCommandBufferAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	allocateInfo.commandBufferCount = 1;
	allocateInfo.commandPool = AstraDevice.getCommandPool();
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	// create
	VkCommandBuffer cmdBuffer;
	vkAllocateCommandBuffers(AstraDevice.getVkDevice(), &allocateInfo, &cmdBuffer);

	CommandList cmdList(cmdBuffer);

	// start
	VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	cmdList.begin(beginInfo);
	return cmdList;
}
