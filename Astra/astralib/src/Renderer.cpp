#include <Renderer.h>
#include <Device.h>
#include <Utils.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

void Astra::Renderer::renderPost(const CommandList& cmdList)
{
	setViewport(cmdList);
	auto aspectRatio = static_cast<float>(_size.width) / static_cast<float>(_size.height);
	_postPipeline.pushConstants(cmdList, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(float), &aspectRatio);
	_postPipeline.bind(cmdList, { _postDescSet });
	cmdList.draw(3, 1, 0, 0);
}

void Astra::Renderer::renderRaster(const CommandList& cmdList, Scene* scene, RasterPipeline* pipeline, const std::vector<VkDescriptorSet>& descSets)
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

	cmdList.beginRenderPass(offscreenRenderPassBeginInfo);

	// dynamic rendering
	VkDeviceSize offset{ 0 };
	setViewport(cmdList);

	// bind pipeline

	pipeline->bind(cmdList, descSets);

	// TODO maybe the push constant should be owned by the app
	// render scene
	PushConstantRaster pushConstant;

	// lights
	for (auto light : scene->getLights())
		light->updatePushConstantRaster(pushConstant);

	// meshes

	// scene.draw(renderingContext);
	// TODO Rendering Context with all draw data needed
	for (auto& inst : scene->getInstances()) {
		// skip invisibles
		if (inst.getVisible()) {
			// get model (with buffers) and update transform matrix
			auto& model = scene->getModels()[inst.getMeshIndex()];
			inst.updatePushConstantRaster(pushConstant);
			pushConstant.modelMatrix = scene->getTransformRef() * pushConstant.modelMatrix;

			// send pc to gpu
			pipeline->pushConstants(cmdList, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				sizeof(PushConstantRaster), &pushConstant);

			// draw call
			model.draw(cmdList);
		}
	}

	// end render pass
	cmdList.endRenderPass();
}

void Astra::Renderer::renderRaytrace(const CommandList& cmdList, Scene* scene, RayTracingPipeline* pipeline, const std::vector<VkDescriptorSet>& descSets)
{
	// push constant info
	PushConstantRay pushConstant{};
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
	for (auto light : scene->getLights())
		light->updatePushConstantRT(pushConstant);

	// TODO si da tiempo habria que pensar una forma de generalizar uniforms
	// probablemente con UBOs para que no haya problemas de tama�o
	pipeline->bind(cmdList, descSets);
	pipeline->pushConstants(cmdList, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
		sizeof(PushConstantRay), &pushConstant);
	cmdList.raytrace(pipeline->getSBTRegions(), _size.width, _size.height);
}

void Astra::Renderer::createPostDescriptorSet()
{
	_postDescSetLayoutBind.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
	_postDescSetLayout = _postDescSetLayoutBind.createLayout(AstraDevice.getVkDevice());
	_postDescPool = _postDescSetLayoutBind.createPool(AstraDevice.getVkDevice());
	_postDescSet = nvvk::allocateDescriptorSet(AstraDevice.getVkDevice(), _postDescPool, _postDescSetLayout);
}

void Astra::Renderer::updatePostDescriptorSet()
{
	VkWriteDescriptorSet writeDescriptorSets = _postDescSetLayoutBind.makeWrite(_postDescSet, 0, &_offscreenColor.descriptor);
	vkUpdateDescriptorSets(AstraDevice.getVkDevice(), 1, &writeDescriptorSets, 0, nullptr);
}


void Astra::Renderer::setViewport(const CommandList& cmdList)
{
	const auto& cmdBuf = cmdList.getCommandBuffer();
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
	const auto& device = AstraDevice.getVkDevice();
	const auto& physicalDevice = AstraDevice.getPhysicalDevice();
	const auto& queue = AstraDevice.getQueue();
	const auto& graphicsQueueIndex = AstraDevice.getGraphicsQueueIndex();
	const auto& commandPool = AstraDevice.getCommandPool();

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

	/*
	* We use the CommandList type in the class attribute
	* However, vulkan expects a VkCommandBuffer array
	* we have a local array of VkCommandBuffer for allocation
	* the proceed to convert it to the CommandList class
	*/
	std::vector<VkCommandBuffer> cmdBufs;
	// Command buffers store a reference to the frame buffer inside their render pass info
	// so for static usage without having to rebuild them each frame, we use one per frame buffer
	VkCommandBufferAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	allocateInfo.commandPool = commandPool;
	allocateInfo.commandBufferCount = _swapchain.getImageCount();
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdBufs.resize(_swapchain.getImageCount());
	vkAllocateCommandBuffers(device, &allocateInfo, cmdBufs.data());
	for (auto& cmdBuf : cmdBufs) {
		_commandLists.push_back({ cmdBuf });
	}

	auto cmdList = Astra::CommandList::createTmpCmdList();
	_swapchain.cmdUpdateBarriers(cmdList.getCommandBuffer());
	cmdList.submitTmpCmdList();
}

void Astra::Renderer::createOffscreenRender(nvvk::ResourceAllocatorDma& alloc)
{
	const auto& device = AstraDevice.getVkDevice();
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
		nvvk::CommandPool genCmdBuf(device, AstraDevice.getGraphicsQueueIndex());
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
		vkDestroyRenderPass(AstraDevice.getVkDevice(), _postRenderPass, nullptr);

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

	vkCreateRenderPass(AstraDevice.getVkDevice(), &renderPassInfo, nullptr, &_postRenderPass);

}

void Astra::Renderer::createPostPipeline()
{
	_postPipeline.createPipeline(AstraDevice.getVkDevice(), { _postDescSetLayout }, _postRenderPass);
}

void Astra::Renderer::getGuiControllerInfo(VkRenderPass& renderpass, int& imageCount, VkFormat& colorFormat, VkFormat& depthFormat)
{
	renderpass = _postRenderPass;
	imageCount = _swapchain.getImageCount();
	colorFormat = _colorFormat;
	depthFormat = _depthFormat;
}

void Astra::Renderer::createFrameBuffers()
{
	// Recreate the frame buffers
	for (auto framebuffer : _framebuffers)
		vkDestroyFramebuffer(AstraDevice.getVkDevice(), framebuffer, nullptr);

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
		vkCreateFramebuffer(AstraDevice.getVkDevice(), &framebufferCreateInfo, nullptr, &_framebuffers[i]);
	}

}

void Astra::Renderer::createDepthBuffer()
{
	const auto& device = AstraDevice.getVkDevice();
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
	memAllocInfo.memoryTypeIndex = AstraDevice.getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	vkAllocateMemory(device, &memAllocInfo, nullptr, &_depthMemory);

	// Bind image and memory
	vkBindImageMemory(device, _depthImage, _depthMemory, 0);

	auto cmdList = Astra::CommandList::createTmpCmdList();

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

	vkCmdPipelineBarrier(cmdList.getCommandBuffer(), srcStageMask, destStageMask, VK_FALSE, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
	cmdList.submitTmpCmdList();


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
	glfwGetFramebufferSize(AstraDevice.getWindow(), &w, &h);

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
	vkWaitForFences(AstraDevice.getVkDevice(), 1, &_fences[imageIndex], VK_TRUE, UINT64_MAX);
}

Astra::CommandList Astra::Renderer::beginFrame()
{
	prepareFrame();
	uint32_t currentFrame = _swapchain.getActiveImageIndex();
	auto& cmdList = _commandLists[currentFrame];

	// begin command buffer
	VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	cmdList.begin(beginInfo);
	return cmdList;
}

void Astra::Renderer::requestSwapchainImage(int w, int h)
{
	_size = _swapchain.update(w, h);
	auto cmdList = Astra::CommandList::createTmpCmdList();
	_swapchain.cmdUpdateBarriers(cmdList.getCommandBuffer());
	cmdList.submitTmpCmdList();

	if (_size.height != h || _size.width != w) {
		Astra::Log("Swapchain image size different from requested one", WARNING);
	}
}

void Astra::Renderer::endFrame(const CommandList& cmdList)
{
	cmdList.end();
	uint32_t imageIndex = _swapchain.getActiveImageIndex();
	vkResetFences(AstraDevice.getVkDevice(), 1, &_fences[imageIndex]);

	const uint32_t                deviceMask = 0b0000'0001;
	const std::array<uint32_t, 2> deviceIndex = { 0, 1 };

	VkDeviceGroupSubmitInfo deviceGroupSubmitInfo{ VK_STRUCTURE_TYPE_DEVICE_GROUP_SUBMIT_INFO_KHR };
	deviceGroupSubmitInfo.waitSemaphoreCount = 1;
	deviceGroupSubmitInfo.commandBufferCount = 1;
	deviceGroupSubmitInfo.pCommandBufferDeviceMasks = &deviceMask;
	deviceGroupSubmitInfo.signalSemaphoreCount = 1;
	deviceGroupSubmitInfo.pSignalSemaphoreDeviceIndices = deviceIndex.data();
	deviceGroupSubmitInfo.pWaitSemaphoreDeviceIndices = deviceIndex.data();

	VkSemaphore semaphoreRead = _swapchain.getActiveReadSemaphore();
	VkSemaphore semaphoreWrite = _swapchain.getActiveWrittenSemaphore();

	// Pipeline stage at which the queue submission will wait (via pWaitSemaphores)
	const VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	std::vector<VkCommandBuffer> commandBuffers(_commandLists.size());
	for (int i = 0; i < commandBuffers.size(); i++) {
		commandBuffers[i] = _commandLists[i].getCommandBuffer();
	}
	// The submit info structure specifies a command buffer queue submission batch
	VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.pWaitDstStageMask = &waitStageMask;  // Pointer to the list of pipeline stages that the semaphore waits will occur at
	submitInfo.pWaitSemaphores = &semaphoreRead;  // Semaphore(s) to wait upon before the submitted command buffer starts executing
	submitInfo.waitSemaphoreCount = 1;                // One wait semaphore
	submitInfo.pSignalSemaphores = &semaphoreWrite;  // Semaphore(s) to be signaled when command buffers have completed
	submitInfo.signalSemaphoreCount = 1;                // One signal semaphore
	submitInfo.pCommandBuffers = &commandBuffers[imageIndex];  // Command buffers(s) to execute in this batch (submission)
	submitInfo.commandBufferCount = 1;                           // One command buffer
	submitInfo.pNext = &deviceGroupSubmitInfo;

	// Submit to the graphics queue passing a wait fence
	vkQueueSubmit(AstraDevice.getQueue(), 1, &submitInfo, _fences[imageIndex]);

	// Presenting frame
	_swapchain.present(AstraDevice.getQueue());
}

void Astra::Renderer::beginPost()
{
	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = { {_clearColor[0], _clearColor[1], _clearColor[2], _clearColor[3]} };
	clearValues[1].depthStencil = { 1.0f, 0 };

	uint32_t currentFrame = _swapchain.getActiveImageIndex();
	auto& cmdBuf = _commandLists[currentFrame];

	VkRenderPassBeginInfo postRenderPassBeginInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	postRenderPassBeginInfo.clearValueCount = 2;
	postRenderPassBeginInfo.pClearValues = clearValues.data();
	postRenderPassBeginInfo.renderPass = _postRenderPass;
	postRenderPassBeginInfo.framebuffer = _framebuffers[currentFrame];
	postRenderPassBeginInfo.renderArea = { {0, 0}, _size };

	cmdBuf.beginRenderPass(postRenderPassBeginInfo);
}

void Astra::Renderer::endPost(const CommandList& cmdList)
{
	cmdList.endRenderPass();
}

void Astra::Renderer::init()
{
	_offscreenDepthFormat = nvvk::findDepthFormat(AstraDevice.getPhysicalDevice());
	_clearColor = glm::vec4(0.8f);
}

void Astra::Renderer::linkApp(App* app)
{
	_app = app;
}

void Astra::Renderer::destroy(nvvk::ResourceAllocator* alloc)
{
	const auto& device = AstraDevice.getVkDevice();
	alloc->destroy(_offscreenColor);
	alloc->destroy(_offscreenDepth);
	vkDestroyImageView(device, _depthView, nullptr);
	vkDestroyImage(device, _depthImage, nullptr);
	vkFreeMemory(device, _depthMemory, nullptr);
	vkDestroyDescriptorPool(device, _postDescPool, nullptr);
	vkDestroyDescriptorSetLayout(device, _postDescSetLayout, nullptr);
	vkDestroyRenderPass(device, _offscreenRenderPass, nullptr);
	vkDestroyRenderPass(device, _postRenderPass, nullptr);
	vkDestroyFramebuffer(device, _offscreenFb, nullptr);
	for (uint32_t i = 0; i < _swapchain.getImageCount(); i++)
	{
		vkDestroyFence(device, _fences[i], nullptr);

		vkDestroyFramebuffer(device, _framebuffers[i], nullptr);

		_commandLists[i].free();
	}
	_swapchain.deinit();
}

void Astra::Renderer::render(const Astra::CommandList& cmdList, Scene* scene, Pipeline* pipeline, const std::vector<VkDescriptorSet>& descSets, Astra::GuiController* gui)
{
	if (pipeline->doesRayTracing()) {
		renderRaytrace(cmdList, scene, (RayTracingPipeline*)pipeline, descSets);
	}
	else {
		renderRaster(cmdList, scene, (RasterPipeline*)pipeline, descSets);
	}

	// post render: ui and texture
	beginPost();
	renderPost(cmdList);
	// render ui
	if (gui != nullptr) {
		gui->startFrame();
		gui->draw(_app);
		gui->endFrame(cmdList);
	}
	endPost(cmdList);

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
VkRenderPass Astra::Renderer::getOffscreenRenderPass() const
{
	return _offscreenRenderPass;
}