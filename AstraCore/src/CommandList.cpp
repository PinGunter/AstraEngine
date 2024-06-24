#include <CommandList.h>
#include <Device.h>

Astra::CommandList::CommandList(const VkCommandBuffer& cmdBuf) : _cmdBuf(cmdBuf) {}

VkCommandBuffer Astra::CommandList::getCommandBuffer() const
{
	return _cmdBuf;
}

void Astra::CommandList::pipelineBarrier(VkPipelineStageFlags srcFlags, VkPipelineStageFlags dstFlags, VkDependencyFlags depsFlags, const std::vector<VkMemoryBarrier>& memoryBarrier, const std::vector<VkBufferMemoryBarrier>& bufferMemoryBarrier, const std::vector<VkImageMemoryBarrier>& imageMemoryBarrier) const
{
	vkCmdPipelineBarrier(_cmdBuf, srcFlags, dstFlags, depsFlags, memoryBarrier.size(), memoryBarrier.data(), bufferMemoryBarrier.size(), bufferMemoryBarrier.data(), imageMemoryBarrier.size(), imageMemoryBarrier.data());
}

void Astra::CommandList::updateBuffer(const nvvk::Buffer& buffer, uint32_t offset, VkDeviceSize size, const void* data) const
{
	vkCmdUpdateBuffer(_cmdBuf, buffer.buffer, offset, size, data);
}

void Astra::CommandList::begin(const VkCommandBufferBeginInfo& beginInfo) const
{
	vkBeginCommandBuffer(_cmdBuf, &beginInfo);
}

void Astra::CommandList::end() const
{
	vkEndCommandBuffer(_cmdBuf);
}

void Astra::CommandList::beginRenderPass(const VkRenderPassBeginInfo& beginInfo, VkSubpassContents subpassContents) const
{
	vkCmdBeginRenderPass(_cmdBuf, &beginInfo, subpassContents);
}

void Astra::CommandList::endRenderPass() const
{
	vkCmdEndRenderPass(_cmdBuf);
}

void Astra::CommandList::drawIndexed(const VkBuffer& vertexBuffer, const VkBuffer& indexBuffer, uint32_t nbIndices) const
{
	VkDeviceSize offset{ 0 };
	vkCmdBindVertexBuffers(_cmdBuf, 0, 1, &vertexBuffer, &offset);
	vkCmdBindIndexBuffer(_cmdBuf, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(_cmdBuf, nbIndices, 1, 0, 0, 0);
}

void Astra::CommandList::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) const
{
	vkCmdDraw(_cmdBuf, vertexCount, instanceCount, firstVertex, firstInstance);
}

void Astra::CommandList::raytrace(const std::array<VkStridedDeviceAddressRegionKHR, 4>& regions, uint32_t width, uint32_t height, uint32_t depth) const
{
	vkCmdTraceRaysKHR(_cmdBuf, &regions[0], &regions[1], &regions[2], &regions[3], width, height, depth);
}

void Astra::CommandList::bindPipeline(PipelineBindPoints bindPoint, const VkPipeline& pipeline) const
{
	vkCmdBindPipeline(_cmdBuf, static_cast<VkPipelineBindPoint>(bindPoint), pipeline);
}

void Astra::CommandList::bindDescriptorSets(PipelineBindPoints bindPoint, const VkPipelineLayout& layout, const std::vector<VkDescriptorSet>& descSets) const
{
	vkCmdBindDescriptorSets(_cmdBuf, static_cast<VkPipelineBindPoint>(bindPoint), layout, 0, static_cast<uint32_t>(descSets.size()), descSets.data(), 0, nullptr);
}


void Astra::CommandList::submitTmpCmdList() const
{
	// end
	vkEndCommandBuffer(_cmdBuf);
	// submit
	VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &_cmdBuf;
	vkQueueSubmit(AstraDevice.getQueue(), 1, &submitInfo, {});
	// sync
	AstraDevice.queueWaitIdle();
	// free
	vkFreeCommandBuffers(AstraDevice.getVkDevice(), AstraDevice.getCommandPool(), 1, &_cmdBuf);
}

void Astra::CommandList::free()
{
	vkFreeCommandBuffers(AstraDevice.getVkDevice(), AstraDevice.getCommandPool(), 1, &_cmdBuf);
}

void Astra::CommandList::pushConstants(const VkPipelineLayout& layout, uint32_t shaderStages, uint32_t size, void* data) const
{
	vkCmdPushConstants(_cmdBuf, layout, shaderStages, 0, size, data);
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
