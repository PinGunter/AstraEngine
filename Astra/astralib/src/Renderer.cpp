#include <Renderer.h>
#include <Device.h>

void Astra::Renderer::renderPost()
{
}

void Astra::Renderer::renderRaster(const Scene& scene, RasterPipeline* pipeline)
{
}

void Astra::Renderer::renderRaytrace(const Scene& scene, RayTracingPipeline* pipeline)
{
}

void Astra::Renderer::createSwapchain(const VkSurfaceKHR& surface, uint32_t width, uint32_t height, VkFormat colorFormat, VkFormat depthFormat)
{
}

void Astra::Renderer::resize(int w, int h)
{
}

void Astra::Renderer::prepareFrame()
{
	int w, h;
	glfwGetFramebufferSize(Astra::Device::getInstance().getWindow(), &w, &h);

	if (w != (int)_size.width || h != (int)_size.height) {
		resize(w, h);
	}

	// acquire swapchain image
	if (!_swapchain.acquire()) {
		throw std::runtime_error("Error acquiring image from swapchain!");
	}

	// use fence to wait for cmdbuff execution
	uint32_t imageIndex = _swapchain.getActiveImageIndex();
	vkWaitForFences(Astra::Device::getInstance().getVkDevice(), 1, &_fences[imageIndex], VK_TRUE, UINT64_MAX);
}

void Astra::Renderer::endFrame()
{
}
	
void Astra::Renderer::render(const Scene& scene, Pipeline* pipeline)
{
	prepareFrame();
	uint32_t currentFrame = _swapchain.getActiveImageIndex();
	const auto& cmdBuff = _commandBuffers[currentFrame];

	// begin command buffer
	VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(cmdBuff, &beginInfo);

	// update camera params
	scene.getCamera()->update(cmdBuff);

	if (pipeline->doesRayTracing()) {
		renderRaytrace(scene, (RayTracingPipeline*)pipeline);
	}
	else {

		renderRaster(scene, (RasterPipeline*)pipeline);
	}
	
	renderPost();
	endFrame();
}
