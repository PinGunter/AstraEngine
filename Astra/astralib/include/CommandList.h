#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <nvvk/resourceallocator_vk.hpp>

namespace Astra {
	class CommandList {
	private:
		VkCommandBuffer _cmdBuf;
	public:
		CommandList(const VkCommandBuffer& cmdBuf);
		VkCommandBuffer getCommandBuffer();

		void pipelineBarrier(VkPipelineStageFlags srcFlags, VkPipelineStageFlags dstFlags, VkDependencyFlags depsFlags, const std::vector<VkMemoryBarrier>& memoryBarrier, const std::vector<VkBufferMemoryBarrier>& bufferMemoryBarrier, const std::vector<VkImageMemoryBarrier>& imageMemoryBarrier);
		void updateBuffer(const nvvk::Buffer& buffer, uint32_t offset, VkDeviceSize size, const void* data);

		void begin(VkCommandBufferBeginInfo beginInfo);

		void submitTmpCmdList();

		static CommandList createTmpCmdList();
	};
}