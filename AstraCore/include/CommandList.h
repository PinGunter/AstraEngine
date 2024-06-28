#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <nvvk/resourceallocator_vk.hpp>
#include <Globals.h>

namespace Astra
{
	/**
	 * @class CommandList
	 * \~spanish @brief Clase que representa un objeto VkCommandBuffer de Vulkan.
	 * Proporciona métodos para realizar las funciones que se han necesitado del commandBuffer. Cada método lo que su nombre indica, solo envuelve la llamada de Vulkan.
	 * \~english @brief Simple class that contains a VkCommandBuffer object
	 * Its methods wrap the command buffer functions used in the library. Every method does as it says, it's just a wrapper around a vulkan function call.
	 */
	class CommandList
	{
	private:
		VkCommandBuffer _cmdBuf;

	public:
		CommandList(const VkCommandBuffer &cmdBuf);
		VkCommandBuffer getCommandBuffer() const;

		void pipelineBarrier(VkPipelineStageFlags srcFlags, VkPipelineStageFlags dstFlags, VkDependencyFlags depsFlags, const std::vector<VkMemoryBarrier> &memoryBarrier, const std::vector<VkBufferMemoryBarrier> &bufferMemoryBarrier, const std::vector<VkImageMemoryBarrier> &imageMemoryBarrier) const;
		void updateBuffer(const nvvk::Buffer &buffer, uint32_t offset, VkDeviceSize size, const void *data) const;

		void begin(const VkCommandBufferBeginInfo &beginInfo) const;
		void end() const;
		void beginRenderPass(const VkRenderPassBeginInfo &beginInfo, VkSubpassContents subpassContents = VK_SUBPASS_CONTENTS_INLINE) const;
		void endRenderPass() const;

		void drawIndexed(const VkBuffer &vertexBuffer, const VkBuffer &indexBuffer, uint32_t nbIndices) const;
		void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex = 0, uint32_t firstInstance = 0) const;
		void raytrace(const std::array<VkStridedDeviceAddressRegionKHR, 4> &regions, uint32_t width, uint32_t height, uint32_t depth = 1) const;
		void bindPipeline(PipelineBindPoints bindPoint, const VkPipeline &pipeline) const;
		void bindDescriptorSets(PipelineBindPoints bindPoint, const VkPipelineLayout &layout, const std::vector<VkDescriptorSet> &descSets) const;
		void pushConstants(const VkPipelineLayout &layout, uint32_t shaderStages, uint32_t size, void *data) const;

		static CommandList createTmpCmdList();
		void submitTmpCmdList() const;

		void free();
	};
}