#include <Renderer.h>
#include <Device.h>
#include <Utils.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

void Astra::Renderer::renderPost(const VkCommandBuffer& cmdBuf)
{
	setViewport(cmdBuf);
	auto aspectRatio = static_cast<float>(_size.width) / static_cast<float>(_size.height);
	_postPipeline.pushConstants(cmdBuf, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(float), &aspectRatio);
	_postPipeline.bind(cmdBuf, { _postDescSet });
	vkCmdDraw(cmdBuf, 3, 1, 0, 0);
}

void Astra::Renderer::renderRaster(const VkCommandBuffer & cmdBuf, const Scene& scene, RasterPipeline* pipeline, const std::vector<VkDescriptorSet>& descSets)
{
	// clear
	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = { _clearColor[0], _clearColor[1], _clearColor[2], _clearColor[3] };
	clearValues[1].depthStencil = { 1.0f, 0 };

	// begin render pass
	VkRenderPassBeginInfo offscreenRenderPassBeginInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	offscreenRenderPassBeginInfo.clearValueCount = 2;
	offscreenRenderPassBeginInfo.pClearValues = clearValues.data();
	offscreenRenderPassBeginInfo.renderPass = _offscreenRenderPass;
	offscreenRenderPassBeginInfo.framebuffer = _offscreenFb;
	offscreenRenderPassBeginInfo.renderArea = { {0, 0}, _size };

	vkCmdBeginRenderPass(cmdBuf, &offscreenRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	// dynamic rendering
	VkDeviceSize offset{ 0 };
	setViewport(cmdBuf);

	// bind pipeline
		
	pipeline->bind(cmdBuf, descSets);

	// render scene
	PushConstantRaster pushConstant;

	// lights
	scene.getLight()->updatePushConstantRaster(pushConstant);

	// meshes

	// scene.draw(renderingContext);
	// TODO Rendering Context with all draw data needed
	for (auto & inst : scene.getInstances()) {
		// skip invisibles
		if (inst.getVisible()) {
			// get model (with buffers) and update transform matrix
			auto& model = scene.getModels()[inst.getMeshIndex()];
			inst.updatePushConstantRaster(pushConstant);
			pushConstant.modelMatrix = scene.getTransformRef() * pushConstant.modelMatrix;

			// send pc to gpu
			pipeline->pushConstants(cmdBuf, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				sizeof(PushConstantRaster), &pushConstant);

			// draw call
			model.draw(cmdBuf, offset);
		}
	}

	// end render pass
	vkCmdEndRenderPass(cmdBuf);
}

void Astra::Renderer::renderRaytrace(const VkCommandBuffer& cmdBuf, const Scene& scene, RayTracingPipeline* pipeline, const std::vector<VkDescriptorSet>& descSets)
{
	// push constant info
	PushConstantRay pushConstant;
	pushConstant.clearColor = _clearColor;
	// lights
	
	// TODO este tipo de cosas me gustaria que fuera mas generico
	// que yo haga un "light->update()" o algo y que se encargara la luz 
	// de manejar sus datos. Asi tambien podria hacer scene.update() y no
	// me importaria si cambia su estructura interna
	// el problema es que el tipo de dato que se envia depende del pipeline
	// ahora mismo esta hecho con dos metodos sobrecargados para admitir
	// los dos tipos de push constant

	/*pushConstant.lightColor = scene.getLight()->getColor();
	pushConstant.lightIntensity = scene.getLight()->getIntensity();
	pushConstant.lightPosition = scene.getLight()->getPosition();
	pushConstant.lightType = scene.getLight()->getType();*/
	scene.getLight()->updatePushConstantRT(pushConstant);

	// TODO si da tiempo habria que pensar una forma de generalizar uniforms
	// probablemente con UBOs para que no haya problemas de tama�o

	pipeline->bind(cmdBuf, descSets);
	pipeline->pushConstants(cmdBuf, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
		sizeof(PushConstantRay), &pushConstant);
	auto regions = pipeline->getSBTRegions();
	vkCmdTraceRaysKHR(cmdBuf, &regions[0], &regions[1], &regions[2], &regions[3], _size.width, _size.height, 1);;
}

void Astra::Renderer::createPostDescriptorSet()
{
	_postDescSetLayoutBind.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
	_postDescSetLayout = _postDescSetLayoutBind.createLayout(Astra::Device::getInstance().getVkDevice());
	_postDescPool = _postDescSetLayoutBind.createPool(Astra::Device::getInstance().getVkDevice());
	_postDescSet = nvvk::allocateDescriptorSet(Astra::Device::getInstance().getVkDevice(), _postDescPool, _postDescSetLayout);
}

void Astra::Renderer::updatePostDescriptorSet()
{
	VkWriteDescriptorSet writeDescriptorSets = _postDescSetLayoutBind.makeWrite(_postDescSet, 0, &_offscreenColor.descriptor);
	vkUpdateDescriptorSets(Astra::Device::getInstance().getVkDevice(), 1, &writeDescriptorSets, 0, nullptr);
}


void Astra::Renderer::setViewport(const VkCommandBuffer& cmdBuf)
{
	VkViewport viewport{ 0.0f, 0.0f, static_cast<float>(_size.width), static_cast<float>(_size.height), 0.0f, 1.0f };
	vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
	VkRect2D scissor{ {0,0}, {_size.width, _size.height} };
	vkCmdSetScissor(cmdBuf, 0, 1, &scissor);
}

void Astra::Renderer::createSwapchain(const VkSurfaceKHR& surface, uint32_t width, uint32_t height, VkFormat colorFormat, VkFormat depthFormat)
{
	_size = VkExtent2D{ width, height };
	_colorFormat = colorFormat;
	_depthFormat = depthFormat;
	const auto& device = Astra::Device::getInstance().getVkDevice();
	const auto& physicalDevice = Astra::Device::getInstance().getPhysicalDevice();
	const auto& queue = Astra::Device::getInstance().getQueue();
	const auto& graphicsQueueIndex = Astra::Device::getInstance().getGraphicsQueueIndex();
	const auto& commandPool = Astra::Device::getInstance().getCommandPool();

	// Find the most suitable depth format
	if (_depthFormat == VK_FORMAT_UNDEFINED)
	{
		auto feature = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
		for (const auto& f : { VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT })
		{
			VkFormatProperties formatProp{ VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2 };
			vkGetPhysicalDeviceFormatProperties(physicalDevice, f, &formatProp);
			if ((formatProp.optimalTilingFeatures & feature) == feature)
			{
				_depthFormat = f;
				break;
			}
		}
	}

	_swapchain.init(device, physicalDevice, queue, graphicsQueueIndex, surface, static_cast<VkFormat>(colorFormat));
	_size = _swapchain.update(_size.width, _size.height, false);
	_colorFormat = static_cast<VkFormat>(_swapchain.getFormat());

	// Create Synchronization Primitives
	_fences.resize(_swapchain.getImageCount());
	for (auto& fence : _fences)
	{
		VkFenceCreateInfo fenceCreateInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		vkCreateFence(device, &fenceCreateInfo, nullptr, &fence);
	}

	// Command buffers store a reference to the frame buffer inside their render pass info
	// so for static usage without having to rebuild them each frame, we use one per frame buffer
	VkCommandBufferAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	allocateInfo.commandPool = commandPool;
	allocateInfo.commandBufferCount = _swapchain.getImageCount();
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	_commandBuffers.resize(_swapchain.getImageCount());
	vkAllocateCommandBuffers(device, &allocateInfo, _commandBuffers.data());

	auto cmdBuffer = Astra::Device::getInstance().createTmpCmdBuf();
	_swapchain.cmdUpdateBarriers(cmdBuffer);
	Astra::Device::getInstance().submitTmpCmdBuf(cmdBuffer);
}

void Astra::Renderer::createOffscreenRender()
{
	auto& alloc = Astra::Device::getInstance().getResAlloc();
	const auto& device = Astra::Device::getInstance().getVkDevice();
	alloc.destroy(_offscreenColor);
	alloc.destroy(_offscreenDepth);

	// Creating the color image
	{
		auto colorCreateInfo = nvvk::makeImage2DCreateInfo(_size, _offscreenColorFormat,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
			| VK_IMAGE_USAGE_STORAGE_BIT);


		nvvk::Image           image = alloc.createImage(colorCreateInfo);
		VkImageViewCreateInfo ivInfo = nvvk::makeImageViewCreateInfo(image.image, colorCreateInfo);
		VkSamplerCreateInfo   sampler{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
		_offscreenColor = alloc.createTexture(image, ivInfo, sampler);
		_offscreenColor.descriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	}

	// Creating the depth buffer
	auto depthCreateInfo = nvvk::makeImage2DCreateInfo(_size, _offscreenDepthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
	{
		nvvk::Image image = alloc.createImage(depthCreateInfo);


		VkImageViewCreateInfo depthStencilView{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
		depthStencilView.format = _offscreenDepthFormat;
		depthStencilView.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };
		depthStencilView.image = image.image;

		_offscreenDepth = alloc.createTexture(image, depthStencilView);
	}

	// Setting the image layout for both color and depth
	{
		nvvk::CommandPool genCmdBuf(device, Astra::Device::getInstance().getGraphicsQueueIndex());
		auto              cmdBuf = genCmdBuf.createCommandBuffer();
		nvvk::cmdBarrierImageLayout(cmdBuf, _offscreenColor.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
		nvvk::cmdBarrierImageLayout(cmdBuf, _offscreenDepth.image, VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);

		genCmdBuf.submitAndWait(cmdBuf);
	}

	// Creating a renderpass for the offscreen
	if (!_offscreenRenderPass)
	{
		_offscreenRenderPass = nvvk::createRenderPass(device, { _offscreenColorFormat }, _offscreenDepthFormat, 1, true,
			true, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);
	}


	// Creating the frame buffer for offscreen
	std::vector<VkImageView> attachments = { _offscreenColor.descriptor.imageView, _offscreenDepth.descriptor.imageView };

	vkDestroyFramebuffer(device, _offscreenFb, nullptr);
	VkFramebufferCreateInfo info{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	info.renderPass = _offscreenRenderPass;
	info.attachmentCount = 2;
	info.pAttachments = attachments.data();
	info.width = _size.width;
	info.height = _size.height;
	info.layers = 1;
	vkCreateFramebuffer(device, &info, nullptr, &_offscreenFb);
}

void Astra::Renderer::createRenderPass()
{
	if (_postRenderPass)
		vkDestroyRenderPass(Astra::Device::getInstance().getVkDevice(), _postRenderPass, nullptr);

	std::array<VkAttachmentDescription, 2> attachments{};
	// Color attachment
	attachments[0].format = _colorFormat;
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;

	// Depth attachment
	attachments[1].format = _depthFormat;
	attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;

	// One color, one depth
	const VkAttachmentReference colorReference{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	const VkAttachmentReference depthReference{ 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

	std::array<VkSubpassDependency, 1> subpassDependencies{};
	// Transition from final to initial (VK_SUBPASS_EXTERNAL refers to all commands executed outside of the actual renderpass)
	subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependencies[0].dstSubpass = 0;
	subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpassDependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkSubpassDescription subpassDescription{};
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorReference;
	subpassDescription.pDepthStencilAttachment = &depthReference;

	VkRenderPassCreateInfo renderPassInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpassDescription;
	renderPassInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
	renderPassInfo.pDependencies = subpassDependencies.data();

	vkCreateRenderPass(Astra::Device::getInstance().getVkDevice(), &renderPassInfo, nullptr, &_postRenderPass);

}

void Astra::Renderer::createFrameBuffers()
{
	// Recreate the frame buffers
	for (auto framebuffer : _framebuffers)
		vkDestroyFramebuffer(Astra::Device::getInstance().getVkDevice(), framebuffer, nullptr);

	// Array of attachment (color, depth)
	std::array<VkImageView, 2> attachments{};

	// Create frame buffers for every swap chain image
	VkFramebufferCreateInfo framebufferCreateInfo{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	framebufferCreateInfo.renderPass = _postRenderPass;
	framebufferCreateInfo.attachmentCount = 2;
	framebufferCreateInfo.width = _size.width;
	framebufferCreateInfo.height = _size.height;
	framebufferCreateInfo.layers = 1;
	framebufferCreateInfo.pAttachments = attachments.data();

	// Create frame buffers for every swap chain image
	_framebuffers.resize(_swapchain.getImageCount());
	for (uint32_t i = 0; i < _swapchain.getImageCount(); i++)
	{
		attachments[0] = _swapchain.getImageView(i);
		attachments[1] = _depthView;
		vkCreateFramebuffer(Astra::Device::getInstance().getVkDevice(), &framebufferCreateInfo, nullptr, &_framebuffers[i]);
	}

}

void Astra::Renderer::createDepthBuffer()
{
	const auto& device = Astra::Device::getInstance().getVkDevice();
	if (_depthView)
		vkDestroyImageView(device, _depthView, nullptr);

	if (_depthImage)
		vkDestroyImage(device, _depthImage, nullptr);

	if (_depthMemory)
		vkFreeMemory(device, _depthMemory, nullptr);

	// Depth information
	const VkImageAspectFlags aspect = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	VkImageCreateInfo        depthStencilCreateInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	depthStencilCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	depthStencilCreateInfo.extent = VkExtent3D{ _size.width, _size.height, 1 };
	depthStencilCreateInfo.format = _depthFormat;
	depthStencilCreateInfo.mipLevels = 1;
	depthStencilCreateInfo.arrayLayers = 1;
	depthStencilCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	depthStencilCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	// Create the depth image
	vkCreateImage(device, &depthStencilCreateInfo, nullptr, &_depthImage);

	// Allocate the memory
	VkMemoryRequirements memReqs;
	vkGetImageMemoryRequirements(device, _depthImage, &memReqs);
	VkMemoryAllocateInfo memAllocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	memAllocInfo.allocationSize = memReqs.size;
	memAllocInfo.memoryTypeIndex = Astra::Device::getInstance().getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	vkAllocateMemory(device, &memAllocInfo, nullptr, &_depthMemory);

	// Bind image and memory
	vkBindImageMemory(device, _depthImage, _depthMemory, 0);

	auto cmdBuffer = Astra::Device::getInstance().createTmpCmdBuf();

	// Put barrier on top, Put barrier inside setup command buffer
	VkImageSubresourceRange subresourceRange{};
	subresourceRange.aspectMask = aspect;
	subresourceRange.levelCount = 1;
	subresourceRange.layerCount = 1;
	VkImageMemoryBarrier imageMemoryBarrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	imageMemoryBarrier.image = _depthImage;
	imageMemoryBarrier.subresourceRange = subresourceRange;
	imageMemoryBarrier.srcAccessMask = VkAccessFlags();
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	const VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	const VkPipelineStageFlags destStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

	vkCmdPipelineBarrier(cmdBuffer, srcStageMask, destStageMask, VK_FALSE, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
	Astra::Device::getInstance().submitTmpCmdBuf(cmdBuffer);


	// Setting up the view
	VkImageViewCreateInfo depthStencilView{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
	depthStencilView.format = _depthFormat;
	depthStencilView.subresourceRange = subresourceRange;
	depthStencilView.image = _depthImage;
	vkCreateImageView(device, &depthStencilView, nullptr, &_depthView);
}

void Astra::Renderer::resize(int w, int h)
{

}

void Astra::Renderer::prepareFrame()
{
	int w, h;
	glfwGetFramebufferSize(Astra::Device::getInstance().getWindow(), &w, &h);

	if (w != (int)_size.width || h != (int)_size.height) {
		// trigger app resize callback
		_app->onResize(w, h);
	}

	// acquire swapchain image
	if (!_swapchain.acquire()) {
		throw std::runtime_error("Error acquiring image from swapchain!");
	}

	// use fence to wait for cmdbuff execution
	uint32_t imageIndex = _swapchain.getActiveImageIndex();
	vkWaitForFences(Astra::Device::getInstance().getVkDevice(), 1, &_fences[imageIndex], VK_TRUE, UINT64_MAX);
}

void Astra::Renderer::requestSwapchainImage(int w, int h)
{
	_size = _swapchain.update(w, h);
	auto cmdBuffer = Astra::Device::getInstance().createTmpCmdBuf();
	_swapchain.cmdUpdateBarriers(cmdBuffer);
	Astra::Device::getInstance().submitTmpCmdBuf(cmdBuffer);

	if (_size.height != h || _size.width != w) {
		Astra::Log("Swapchain image size different from requested one", WARNING);
	}
}

void Astra::Renderer::endFrame()
{
	uint32_t imageIndex = _swapchain.getActiveImageIndex();
	vkResetFences(Astra::Device::getInstance().getVkDevice(), 1, &_fences[imageIndex]);

	const uint32_t                deviceMask = 0b0000'0001;
	const std::array<uint32_t, 2> deviceIndex = { 0, 1 };

	VkDeviceGroupSubmitInfo deviceGroupSubmitInfo{ VK_STRUCTURE_TYPE_DEVICE_GROUP_SUBMIT_INFO_KHR };
	deviceGroupSubmitInfo.waitSemaphoreCount = 1;
	deviceGroupSubmitInfo.commandBufferCount = 1;
	deviceGroupSubmitInfo.pCommandBufferDeviceMasks = &deviceMask;
	deviceGroupSubmitInfo.signalSemaphoreCount =  1;
	deviceGroupSubmitInfo.pSignalSemaphoreDeviceIndices = deviceIndex.data();
	deviceGroupSubmitInfo.pWaitSemaphoreDeviceIndices = deviceIndex.data();

	VkSemaphore semaphoreRead = _swapchain.getActiveReadSemaphore();
	VkSemaphore semaphoreWrite = _swapchain.getActiveWrittenSemaphore();

	// Pipeline stage at which the queue submission will wait (via pWaitSemaphores)
	const VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	// The submit info structure specifies a command buffer queue submission batch
	VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.pWaitDstStageMask = &waitStageMask;  // Pointer to the list of pipeline stages that the semaphore waits will occur at
	submitInfo.pWaitSemaphores = &semaphoreRead;  // Semaphore(s) to wait upon before the submitted command buffer starts executing
	submitInfo.waitSemaphoreCount = 1;                // One wait semaphore
	submitInfo.pSignalSemaphores = &semaphoreWrite;  // Semaphore(s) to be signaled when command buffers have completed
	submitInfo.signalSemaphoreCount = 1;                // One signal semaphore
	submitInfo.pCommandBuffers = &_commandBuffers[imageIndex];  // Command buffers(s) to execute in this batch (submission)
	submitInfo.commandBufferCount = 1;                           // One command buffer
	submitInfo.pNext = &deviceGroupSubmitInfo;

	// Submit to the graphics queue passing a wait fence
	vkQueueSubmit(Astra::Device::getInstance().getQueue(), 1, &submitInfo, _fences[imageIndex]);

	// Presenting frame
	_swapchain.present(Astra::Device::getInstance().getQueue());
}

void Astra::Renderer::updateUBO(const VkCommandBuffer& cmdBuf, const GlobalUniforms& hostUbo)
{
	VkBuffer deviceUBO = _app->getCameraUBO().buffer;
	auto uboStages = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;

	// sync (new UBO not visible to previous frames)
	VkBufferMemoryBarrier beforeBarrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
	beforeBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	beforeBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	beforeBarrier.buffer = deviceUBO;
	beforeBarrier.offset = 0;
	beforeBarrier.size = sizeof(hostUbo);
	vkCmdPipelineBarrier(cmdBuf, uboStages, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_DEVICE_GROUP_BIT,
		0, nullptr, 1, &beforeBarrier, 0, nullptr);

	// copy to device
	vkCmdUpdateBuffer(cmdBuf, _app->getCameraUBO().buffer, 0, sizeof(GlobalUniforms), &hostUbo);

	// sync (making the UBO visible)
	VkBufferMemoryBarrier afterBarrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
	afterBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	afterBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	afterBarrier.buffer = deviceUBO;
	afterBarrier.offset = 0;
	afterBarrier.size = sizeof(GlobalUniforms);
	vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TRANSFER_BIT, uboStages,
		VK_DEPENDENCY_DEVICE_GROUP_BIT, 0, nullptr, 1, &afterBarrier, 0, nullptr);
}
	
void Astra::Renderer::init(App* app)
{
	_app = app;
	_offscreenDepthFormat = nvvk::findDepthFormat(Astra::Device::getInstance().getPhysicalDevice());

}

void Astra::Renderer::destroy()
{
	auto& alloc = Astra::Device::getInstance().getResAlloc();
	const auto& device = Astra::Device::getInstance().getVkDevice();
	alloc.destroy(_offscreenColor);
	alloc.destroy(_offscreenDepth);
	vkDestroyDescriptorPool(device, _postDescPool, nullptr);
	vkDestroyDescriptorSetLayout(device, _postDescSetLayout, nullptr);
	vkDestroyRenderPass(device, _offscreenRenderPass, nullptr);
	vkDestroyRenderPass(device, _postRenderPass, nullptr);
	vkDestroyFramebuffer(device, _offscreenFb, nullptr);

}

void Astra::Renderer::render(const Scene& scene, Pipeline* pipeline, const std::vector<VkDescriptorSet> & descSets)
{
	prepareFrame();
	uint32_t currentFrame = _swapchain.getActiveImageIndex();
	const auto& cmdBuf = _commandBuffers[currentFrame];

	// begin command buffer
	VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(cmdBuf, &beginInfo);

	if (pipeline->doesRayTracing()) {
		renderRaytrace(cmdBuf, scene, (RayTracingPipeline*)pipeline, descSets);
	}
	else {
		renderRaster(cmdBuf, scene, (RasterPipeline*)pipeline, descSets);
	}
	
	renderPost(cmdBuf);
	endFrame();
}

glm::vec4& Astra::Renderer::getClearColorRef()
{
	return _clearColor;
}

glm::vec4 Astra::Renderer::getClearColor() const
{
	return _clearColor;
}

void Astra::Renderer::setClearColor(const glm::vec4& color)
{
	_clearColor = color;
}
const nvvk::Texture& Astra::Renderer::getOffscreenColor() const
{
	return _offscreenColor;
}
//
//void Astra::Renderer::initGUIRendering(Gui& gui)
//{
//	const auto& device = Astra::Device::getInstance().getVkDevice();
//	const auto& physicalDevice = Astra::Device::getInstance().getPhysicalDevice();
//	const auto& instance = Astra::Device::getInstance().getVkInstance();
//	const auto& queueFamily = Astra::Device::getInstance().getGraphicsQueueIndex();
//	const auto& queue = Astra::Device::getInstance().getQueue();
//	ImGui::CreateContext();
//	ImGuiIO& io = ImGui::GetIO();
//	//io.IniFilename = nullptr;  // Avoiding the INI file
//	io.LogFilename = nullptr;
//	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
//	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // Enable Docking
//	//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;    // Enable Multi-Viewport / Platform Windows
//
//	//ImGuiH::setStyle();
//	//ImGuiH::setFonts();
//
//	std::vector<VkDescriptorPoolSize> poolSize{ {VK_DESCRIPTOR_TYPE_SAMPLER, 1}, {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1} };
//	VkDescriptorPoolCreateInfo poolInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
//	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
//	poolInfo.maxSets = 1000;
//	poolInfo.poolSizeCount = 2;
//	poolInfo.pPoolSizes = poolSize.data();
//	vkCreateDescriptorPool(device, &poolInfo, nullptr, &gui._imguiDescPool);
//
//	// Setup Platform/Renderer back ends
//	ImGui_ImplVulkan_InitInfo init_info = {};
//	init_info.Instance = instance;
//	init_info.PhysicalDevice = physicalDevice;
//	init_info.Device = device;
//	init_info.QueueFamily = queueFamily;
//	init_info.Queue = queue;
//	init_info.PipelineCache = VK_NULL_HANDLE;
//	init_info.DescriptorPool = gui._imguiDescPool;
//	init_info.RenderPass = _postRenderPass;
//	init_info.Subpass = 0;
//	init_info.MinImageCount = 2;
//	init_info.ImageCount = static_cast<int>(_swapchain.getImageCount());
//	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT; 
//	init_info.CheckVkResultFn = nullptr;
//	init_info.Allocator = nullptr;
//
//	init_info.UseDynamicRendering = false;
//	init_info.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
//	init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
//	init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &_colorFormat;
//	init_info.PipelineRenderingCreateInfo.depthAttachmentFormat = _depthFormat;
//
//	ImGui_ImplVulkan_Init(&init_info);
//
//	// Upload Fonts
//	ImGui_ImplVulkan_CreateFontsTexture();
//
//	ImGui_ImplGlfw_InitForVulkan(Astra::Device::getInstance().getWindow(), true);
//
//}
