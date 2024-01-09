#include "VulkanApp.h"
#include <stdexcept>
#include <map>
#include <set>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>

#define STB_IMAGE_IMPLEMENTATION

#include <stb_image.h>

std::vector<char> readFile(const std::string &filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary); // start reading at the end so we now file size
    if (!file.is_open()) {
        throw std::runtime_error("failed to open file! (" + filename + ")");
    }
    // allocate for correct file size
    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0); // back to the begginning to read bytes
    file.read(buffer.data(), fileSize);
    std::cout << filename << ": " << fileSize << " bytes" << std::endl;
    file.close();
    return buffer;
}

void VulkanApp::run() {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

void VulkanApp::initWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan App", nullptr, nullptr);

    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizedCallback);
}

void VulkanApp::framebufferResizedCallback(GLFWwindow *window, int width, int height) {
    auto app = reinterpret_cast<VulkanApp *>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
}

void VulkanApp::initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapchain();
    createImageViews();
    createRenderPass();
    createDescriptorSetLayout();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandPool();
    createTextureImage();
    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
    createSyncObjects();
}

void VulkanApp::mainLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        drawFrame();
    }
    device->waitIdle();
}

void VulkanApp::cleanup() {
    cleanupSwapchain();

    device->destroyImage(textureImage);
    device->freeMemory(textureImageMemory);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        device->destroyBuffer(uniformBuffers[i], nullptr);
        device->freeMemory(uniformBuffersMemory[i], nullptr);
    }

    device->destroyDescriptorPool(descriptorPool, nullptr);
    device->destroyDescriptorSetLayout(descriptorSetLayout, nullptr);

    device->destroyBuffer(vertexBuffer, nullptr);
    device->freeMemory(vertexBufferMemory, nullptr);
    device->destroyBuffer(indexBuffer, nullptr);
    device->freeMemory(indexBufferMemory, nullptr);

    device->destroyPipeline(graphicsPipeline, nullptr);
    device->destroyPipelineLayout(pipelineLayout, nullptr);
    device->destroyRenderPass(renderPass, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        device->destroySemaphore(renderFinishedSemaphores[i], nullptr);
        device->destroySemaphore(imageAvailableSemaphores[i], nullptr);
        device->destroyFence(inFlightFences[i], nullptr);
    }

    device->destroyCommandPool(commandPool, nullptr);

    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(nullptr);
    }

    instance->destroySurfaceKHR(surface, nullptr);

    glfwDestroyWindow(window);

    glfwTerminate();
}

void VulkanApp::createInstance() {
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }

    vk::ApplicationInfo appInfo = vk::ApplicationInfo(
            "Vulkan App",
            VK_MAKE_VERSION(1, 0, 0),
            "No Engine",
            VK_MAKE_VERSION(1, 0, 0),
            VK_API_VERSION_1_0);

    auto createInfo = vk::InstanceCreateInfo(vk::InstanceCreateFlags(), &appInfo);

    auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

        auto debugCreateInfo = populateDebugMessengerCreateInfo();
        createInfo.pNext = (vk::DebugUtilsMessengerCreateInfoEXT *) &debugCreateInfo;
    } else {
        createInfo.enabledExtensionCount = 0;
        createInfo.pNext = nullptr;
    }

    instance = vk::createInstanceUnique(createInfo);
}

vk::DebugUtilsMessengerCreateInfoEXT VulkanApp::populateDebugMessengerCreateInfo() {
    auto severityFlags =
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
    /*| vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo*/;

    auto typeFlags =
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
            vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;

    vk::DebugUtilsMessengerCreateInfoEXT messengerCreateInfo;
    messengerCreateInfo.flags = vk::DebugUtilsMessengerCreateFlagBitsEXT();
    messengerCreateInfo.messageSeverity = severityFlags;
    messengerCreateInfo.messageType = typeFlags;
    messengerCreateInfo.pfnUserCallback = debugCallback;

    return messengerCreateInfo;
}

void VulkanApp::setupDebugMessenger() {
    auto createInfo = populateDebugMessengerCreateInfo();
    vk::DispatchLoaderDynamic instanceLoader(*instance, vkGetInstanceProcAddr);
    debugMessenger = instance->createDebugUtilsMessengerEXT(createInfo, nullptr, instanceLoader);
}

void VulkanApp::createSurface() {
    VkSurfaceKHR surf;
    if (glfwCreateWindowSurface(static_cast<VkInstance>(*instance), window, nullptr, &surf) != VK_SUCCESS) {
        throw std::runtime_error("failed to create surface");
    }
    surface = surf;
}

void VulkanApp::pickPhysicalDevice() {
    auto physicalDevices = instance->enumeratePhysicalDevices();
    if (physicalDevices.size() == 0) {
        throw std::runtime_error("failed to find any GPUs with vulkan support!");
    }
    std::multimap<int, vk::PhysicalDevice> candidates;

    for (const auto &device: physicalDevices) {
        int score = rateDeviceSuitability(device);
        candidates.insert(std::make_pair(score, device));
    }

    if (candidates.rbegin()->first > 0) {
        physicalDevice = candidates.rbegin()->second;
    } else {
        throw std::runtime_error("failed to find any suitable GPU");
    }
}

void VulkanApp::createLogicalDevice() {
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    float queuePriority = 1.0f;
    for (uint32_t queueFamily: uniqueQueueFamilies) {
        vk::DeviceQueueCreateInfo queueCreateInfo(
                {},                 // flags
                queueFamily,     // queueFamilyIndex
                1,                 // queueCount
                &queuePriority); // pQueuePriorities
        queueCreateInfos.push_back(queueCreateInfo);
    }

    vk::DeviceCreateInfo createInfo({},
                                    static_cast<uint32_t>(queueCreateInfos.size()),                                 // queueCreateInfoCount
                                    queueCreateInfos.data(),                                                     // pQueueCreateInfos
                                    enableValidationLayers ? static_cast<uint32_t>(validationLayers.size())
                                                           : 0, // layer count
                                    enableValidationLayers ? validationLayers.data()
                                                           : nullptr,                     // layer ptr
                                    static_cast<uint32_t>(deviceExtensions.size()),                                 // extensions count
                                    deviceExtensions.data()                                                         // extensions ptr
    );

    device = physicalDevice.createDeviceUnique(createInfo);
    graphicsQueue = device->getQueue(indices.graphicsFamily.value(), 0);
    presentQueue = device->getQueue(indices.presentFamily.value(), 0);
}

void VulkanApp::cleanupSwapchain() {
    for (auto fb: swapchainFramebuffers) {
        device->destroyFramebuffer(fb, nullptr);
    }

    for (auto iv: swapchainImageViews) {
        device->destroyImageView(iv, nullptr);
    }

    device->destroySwapchainKHR(swapchain, nullptr);
}

void VulkanApp::recreateSwapchain() {
    // handling minimization (width and height are 0)
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    device->waitIdle();
    cleanupSwapchain();

    createSwapchain();
    createImageViews();
    createFramebuffers();
}

void VulkanApp::createSwapchain() {
    SwapchainSupportDetails swapchainSupport = querySwapchainSupport(physicalDevice);

    vk::SurfaceFormatKHR surfaceFormat = chooseSwapchainSurfaceFormat(swapchainSupport.formats);
    vk::PresentModeKHR presentMode = chooseSwapchainPresentMode(swapchainSupport.presentModes);
    vk::Extent2D extent = chooseSwapExtend(swapchainSupport.capabilities);

    // at least 1 more than the minimun so there is less idle time
    uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;

    // 0 is a special value that means that there is no maximum
    if (swapchainSupport.capabilities.maxImageCount > 0 && imageCount > swapchainSupport.capabilities.maxImageCount) {
        imageCount = swapchainSupport.capabilities.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR createInfo(
            {},                          // flags
            surface,                  // surface
            imageCount,                  // minImageCount
            surfaceFormat.format,      // format
            surfaceFormat.colorSpace, // colorSpace
            extent,                      // extent
            1,                          // image array layers
            /*
             * This bit specifies what kind of operation are the images generated in the swapchain used for.
             * As of now, the images are going to be rendered directly into them => they are used as color attachment
             * We could render images to a separate image first to perform operations like post-processing
             * In that case, maybe using VK_IMAGE_USAGE_TRANSFER_DST_BIT
             * and then use a memory operation to transfer the rendered image to the swapchain
             */
            vk::ImageUsageFlagBits::eColorAttachment // image usage
    );

    /*
     * We need to specify how to handle swapchain images in the different queue families
     * We will draw the image in the swapchain from the graphics queue and then submit it to the presentation queue
     *
     * There are two ways to handle that:
     * * CONCURRENT: images can be used across multiple queues without explicit ownership transfers
     * * EXCLUSIVE: an image is owned by one queue family at a time. Ownership must be explicitly transferred. Better performance
     *
     * As of now, we'll stick to concurrent so that we don't have to handle ownership transfers
     */
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = vk::SharingMode::eExclusive;
        createInfo.queueFamilyIndexCount = 0;      // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    createInfo.preTransform = swapchainSupport.capabilities.currentTransform; // no transformations

    createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;

    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    createInfo.oldSwapchain = nullptr; // Future

    swapchain = device->createSwapchainKHR(createInfo);
    swapchainImages = device->getSwapchainImagesKHR(swapchain);

    swapchainImageFormat = surfaceFormat.format;
    swapchainExtent = extent;
}

void VulkanApp::createImageViews() {
    swapchainImageViews.resize(swapchainImages.size());

    for (size_t i = 0; i < swapchainImageViews.size(); i++) {
        vk::ImageViewCreateInfo createInfo(
                {},                        // flags
                swapchainImages[i],        // image
                vk::ImageViewType::e2D, // viewType
                swapchainImageFormat,    // format
                {
                        vk::ComponentSwizzle::eIdentity, // components.r
                        vk::ComponentSwizzle::eIdentity, // components.g
                        vk::ComponentSwizzle::eIdentity, // components.b
                        vk::ComponentSwizzle::eIdentity     // components.a
                },
                {
                        vk::ImageAspectFlagBits::eColor, // subresourceRange.aspectMask
                        0,                                 // subresourceRange.baseMipmaplevel
                        1,                                 // subresourceRange.levelCount
                        0,                                 // subresourceRange.baseArrayLayer
                        1                                 // subresourceRange.layerCount
                });
        swapchainImageViews[i] = device->createImageView(createInfo);
    }
}

void VulkanApp::createRenderPass() {
    vk::AttachmentDescription colorAttachment(
            {},                                  // flags
            swapchainImageFormat,              // format
            vk::SampleCountFlagBits::e1,      // samples
            vk::AttachmentLoadOp::eClear,      // load op
            vk::AttachmentStoreOp::eStore,      // store op
            vk::AttachmentLoadOp::eDontCare,  // stencil load op
            vk::AttachmentStoreOp::eDontCare, // stencil store op
            vk::ImageLayout::eUndefined,      // initial layout
            vk::ImageLayout::ePresentSrcKHR      // final layout
    );

    vk::AttachmentReference colorAttachmentRef(
            0,                                         // attachment -> index of attachment, since we only have 1 its 0
            vk::ImageLayout::eColorAttachmentOptimal // layout
    );

    vk::SubpassDescription subpass;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    // make the renderpass wait until image is acquired
    vk::SubpassDependency dependency;
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.srcAccessMask = {};
    dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

    vk::RenderPassCreateInfo createInfo(
            {},                  // flags
            1,                  // attachment count
            &colorAttachment, // attachment ptr
            1,                  // subpass count
            &subpass,          // subpass ptr
            1,                  // dependency count
            &dependency          // dependency ptr
    );

    renderPass = device->createRenderPass(createInfo);
}

void VulkanApp::createDescriptorSetLayout() {
    vk::DescriptorSetLayoutBinding uboLayoutBinding(
            0,                                    // binding -> (binding = 0) in shader
            vk::DescriptorType::eUniformBuffer, // descriptor type
            1,                                    // descriptor count
            vk::ShaderStageFlagBits::eVertex,    // stage flags
            nullptr                                // pImmutableSamplers
    );

    vk::DescriptorSetLayoutCreateInfo layoutInfo(
            {},                  // flags
            1,                  // binding count
            &uboLayoutBinding // pBindings
    );

    descriptorSetLayout = device->createDescriptorSetLayout(layoutInfo);
}

void VulkanApp::createGraphicsPipeline() {
    // read the bytecode
    auto vertShaderCode = readFile("shaders/shader_vert.spv");
    auto fragShaderCode = readFile("shaders/shader_frag.spv");

    // wrap it in vk::ShaderModule
    vk::ShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    vk::ShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    // create the shader stages for each shader
    vk::PipelineShaderStageCreateInfo vertShaderStageInfo(
            {},                                  // flags
            vk::ShaderStageFlagBits::eVertex, // stage
            vertShaderModule,                  // module
            "main"                              // pName
    );

    vk::PipelineShaderStageCreateInfo fragShaderStageInfo(
            {},                                    // flags
            vk::ShaderStageFlagBits::eFragment, // stage
            fragShaderModule,                    // module
            "main"                                // pName
    );

    vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    // fixed functions

    auto bindingDescriptions = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo(
            {},                                                     // flags
            1,                                                     // vertexBindingDescription count
            &bindingDescriptions,                                 // vertexBindingDescription ptr
            static_cast<uint32_t>(attributeDescriptions.size()), // vertexAttributeDescription count
            attributeDescriptions.data()                         // vertexAttributeDescription ptr
    );

    // Input assembly (geometry from vertices[primitives])
    /*
     * (some) Possible options
     * ePointList -> points
     * eLineList -> lines (no reuse)
     * eLineStrip -> lines (reusing end -> beginning)
     * eTriangleList -> triangle from every 3 vertices || classic
     * eTriangleStrip -> triangle strip
     */
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly(
            {},                                      // flags
            vk::PrimitiveTopology::eTriangleList, // topology
            VK_FALSE                              // primite restart enablk
    );

    vk::Viewport viewport(
            0.0f, 0.0f, (float) swapchainExtent.width, (float) swapchainExtent.height, 0.0f,
            1.0f // x, y, width, height, minDepth, maxDepth
    );

    vk::Rect2D scissor(
            {0, 0}, swapchainExtent // offset, extent
    );

    // we are going to have a dynamic state so that we can change the viewport or scissor size during runtime
    std::vector<vk::DynamicState> dynamicStates = {
            vk::DynamicState::eViewport, vk::DynamicState::eScissor};

    vk::PipelineDynamicStateCreateInfo dynamicState(
            {},                                             // flags
            static_cast<uint32_t>(dynamicStates.size()), // dynamic states count
            dynamicStates.data()                         // dynamic states ptr
    );

    vk::PipelineViewportStateCreateInfo viewportState(
            {},         // flags
            1,         // viewport count
            nullptr, // viewport ptr (nullptr as we'll set it dynamically)
            1,         // scissor count
            nullptr     // scissor ptr (nullptr as we'll set it dynamically)
    );

    vk::PipelineRasterizationStateCreateInfo rasterizer(
            {},                                  // flags
            VK_FALSE,                          // depthClampEnable -> clamps objects outside of near/far to these planes
            VK_FALSE,                          // rasterizerDiscardEnable -> if true no geometry passes through the rasterizer
            vk::PolygonMode::eFill,              // polygonMode
            vk::CullModeFlagBits::eBack,      // cullMode
            vk::FrontFace::eCounterClockwise, // frontFace
            VK_FALSE,                          // deptBiasEnable
            0.0f,                              // depthBiasConstantFactor
            0.0f,                              // depthBiasClamp
            0.0f,                              // depthBiasSlopeFactor
            1.0f                              // lineWidth
    );

    vk::PipelineMultisampleStateCreateInfo multisampling(
            {},                             // flags
            vk::SampleCountFlagBits::e1, // rasterization samples
            VK_FALSE,                     // sampleShadingEnable
            1.0f,                         // minSampleShading
            nullptr,                     // pSampleMask
            VK_FALSE,                     // alphatoOneCoverageEnable
            VK_FALSE                     // alphatoOneEnable
    );

    // depth and stencil test
    // currently disabled

    // color blending
    // attachmentState-> per framebuffer
    // createinfo -> global
    vk::PipelineColorBlendAttachmentState colorBlendAttachment(VK_FALSE);
    colorBlendAttachment.colorWriteMask =
            vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
            vk::ColorComponentFlagBits::eA;

    vk::PipelineColorBlendStateCreateInfo colorBlending(
            {},                       // flags
            VK_FALSE,               // logicOpEnable
            vk::LogicOp::eCopy,       // logicOp
            1,                       // attachment count
            &colorBlendAttachment, // attachment ptr
            {
                    0.0f, 0.0f, 0.0f, 0.0f // blend constants
            });

    // pipeline layout
    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo(
            {},                     // flags
            1,                     // setlayout count
            &descriptorSetLayout // setlayout ptr
    );

    pipelineLayout = device->createPipelineLayout(pipelineLayoutCreateInfo);

    vk::GraphicsPipelineCreateInfo pipelineInfo(
            {},                  // flags
            2,                  // stage count
            shaderStages,      // shader stages
            &vertexInputInfo, // vertex input state ptr
            &inputAssembly,      // input assembly state ptr
            nullptr,          // tesselation state ptr
            &viewportState,      // viewport state ptr
            &rasterizer,      // rasterization state ptr
            &multisampling,      // multisample state ptr
            nullptr,          // depth stencil state ptr
            &colorBlending,      // color blend state ptr
            &dynamicState,      // dynamic state ptr
            pipelineLayout,      // layout
            renderPass,          // renderpass
            0,                  // subpass
            nullptr,          // basepipelinehandle
            -1                  // basePipelineindex
    );

    graphicsPipeline = device->createGraphicsPipeline(nullptr, pipelineInfo).value;

    device->destroyShaderModule(vertShaderModule);
    device->destroyShaderModule(fragShaderModule);
}

void VulkanApp::createFramebuffers() {
    swapchainFramebuffers.resize(swapchainImageViews.size());

    for (size_t i = 0; i < swapchainImageViews.size(); i++) {
        vk::ImageView attachments[] = {swapchainImageViews[i]};
        vk::FramebufferCreateInfo createInfo(
                {},                        // flags
                renderPass,                // renderpass
                1,                        // attachment count
                attachments,            // attachments ptr
                swapchainExtent.width,    // width
                swapchainExtent.height, // height
                1);
        swapchainFramebuffers[i] = device->createFramebuffer(createInfo);
    }
}

void VulkanApp::createCommandPool() {
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);
    vk::CommandPoolCreateInfo poolInfo(
            vk::CommandPoolCreateFlagBits::eResetCommandBuffer, // flag
            queueFamilyIndices.graphicsFamily.value()            // queueFamilyIndex
    );
    commandPool = device->createCommandPool(poolInfo);
}

void VulkanApp::createTextureImage() {
    int texWidth, texHeight, texChannels;
    stbi_uc *pixels = stbi_load("textures/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    vk::DeviceSize imageSize = texWidth * texHeight * 4;
    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }
    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;

    createBuffer(
            imageSize,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible,
            stagingBuffer,
            stagingBufferMemory
    );

    void *data = device->mapMemory(stagingBufferMemory, 0, imageSize);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    device->unmapMemory(stagingBufferMemory);
    stbi_image_free(pixels);

    createImage(texWidth, texHeight, vk::Format::eR8G8B8A8Srgb, vk::ImageTiling::eOptimal,
                vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
                vk::MemoryPropertyFlagBits::eDeviceLocal, textureImage, textureImageMemory);

    transitionImageLayout(textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eUndefined,
                          vk::ImageLayout::eTransferDstOptimal);
    copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t> (texWidth), static_cast<uint32_t> (texHeight));
    transitionImageLayout(textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eTransferDstOptimal,
                          vk::ImageLayout::eShaderReadOnlyOptimal);


    device->destroyBuffer(stagingBuffer, nullptr);
    device->freeMemory(stagingBufferMemory, nullptr);
}

vk::CommandBuffer VulkanApp::beginSingleTimeCommands() {
    vk::CommandBufferAllocateInfo allocInfo(
            commandPool,
            vk::CommandBufferLevel::ePrimary,
            1);
    vk::CommandBuffer commandBuffer = device->allocateCommandBuffers(allocInfo)[0];
    vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    commandBuffer.begin(beginInfo);
    return commandBuffer;
}

void VulkanApp::endSingleTimeCommands(vk::CommandBuffer commandBuffer) {
    commandBuffer.end();
    vk::SubmitInfo submitInfo;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    graphicsQueue.submit(submitInfo);
    graphicsQueue.waitIdle();
    device->freeCommandBuffers(commandPool, 1, &commandBuffer);
}

void VulkanApp::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties,
                             vk::Buffer &buffer, vk::DeviceMemory &bufferMemory) {
    vk::BufferCreateInfo bufferInfo(
            {},                            // flags
            size,                        // size
            usage,                        // usage
            vk::SharingMode::eExclusive // sharing mode
    );

    buffer = device->createBuffer(bufferInfo);

    vk::MemoryRequirements memRequirements = device->getBufferMemoryRequirements(buffer);

    vk::MemoryAllocateInfo allocInfo(
            memRequirements.size,                                       // allocationSize
            findMemoryType(memRequirements.memoryTypeBits, properties) // memoryTypeIndex
    );

    bufferMemory = device->allocateMemory(allocInfo);
    device->bindBufferMemory(buffer, bufferMemory, 0);
}

void VulkanApp::createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling,
                            vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Image &image,
                            vk::DeviceMemory &imageMemory) {
    vk::ImageCreateInfo imageInfo(
            {},
            vk::ImageType::e2D,
            format
    );
    imageInfo.extent.width = static_cast<uint32_t> (width);
    imageInfo.extent.height = static_cast<uint32_t> (height);
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = vk::ImageLayout::eUndefined;
    imageInfo.usage = usage;
    imageInfo.sharingMode = vk::SharingMode::eExclusive;
    imageInfo.samples = vk::SampleCountFlagBits::e1;

    image = device->createImage(imageInfo);

    vk::MemoryRequirements memRequirements = device->getImageMemoryRequirements(image);
    vk::MemoryAllocateInfo allocInfo(
            memRequirements.size,
            findMemoryType(memRequirements.memoryTypeBits, properties)
    );
    imageMemory = device->allocateMemory(allocInfo);

    device->bindImageMemory(image, imageMemory, 0);
}

void VulkanApp::copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size) {
    vk::CommandBuffer commandBuffer = beginSingleTimeCommands();
    vk::BufferCopy copyRegion(0, 0, size);
    commandBuffer.copyBuffer(srcBuffer, dstBuffer, 1, &copyRegion);
    endSingleTimeCommands(commandBuffer);
}

void VulkanApp::transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout,
                                      vk::ImageLayout newLayout) {
    vk::CommandBuffer commandBuffer = beginSingleTimeCommands();

    vk::ImageMemoryBarrier barrier(
            {},
            {},
            oldLayout,
            newLayout,
            vk::QueueFamilyIgnored,
            vk::QueueFamilyIgnored,
            image
    );
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;


    vk::PipelineStageFlags sourceStage, destinationStage;

    if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
        barrier.srcAccessMask = vk::AccessFlagBits::eNone;
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
    } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal &&
               newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
        sourceStage = vk::PipelineStageFlagBits::eTransfer;
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
    } else {
        throw std::runtime_error("unsupported layout transition");
    }

    commandBuffer.pipelineBarrier(sourceStage, destinationStage,
                                  {},
                                  0, nullptr,
                                  0, nullptr,
                                  1, &barrier);

    endSingleTimeCommands(commandBuffer);
}

void VulkanApp::copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height) {
    vk::CommandBuffer commandBuffer = beginSingleTimeCommands();

    vk::BufferImageCopy region(0, 0, 0);
    region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = vk::Offset3D{0, 0, 0};
    region.imageExtent = vk::Extent3D{
            width,
            height,
            1
    };
    commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, region);
    endSingleTimeCommands(commandBuffer);
}

void VulkanApp::createVertexBuffer() {
    vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;

    createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
                 vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer,
                 stagingBufferMemory);

    // mapping the buffer memory to cpu memory
    void *data = device->mapMemory(stagingBufferMemory, 0, bufferSize);
    memcpy(data, vertices.data(), (size_t) bufferSize);
    device->unmapMemory(stagingBufferMemory);

    createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
                 vk::MemoryPropertyFlagBits::eDeviceLocal, vertexBuffer, vertexBufferMemory);
    copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
    device->destroyBuffer(stagingBuffer);
    device->freeMemory(stagingBufferMemory);
}

void VulkanApp::createIndexBuffer() {
    vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();
    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;

    createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
                 vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer,
                 stagingBufferMemory);

    // mapping the buffer memory to cpu memory
    void *data = device->mapMemory(stagingBufferMemory, 0, bufferSize);
    memcpy(data, indices.data(), (size_t) bufferSize);
    device->unmapMemory(stagingBufferMemory);

    createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
                 vk::MemoryPropertyFlagBits::eDeviceLocal, indexBuffer, indexBufferMemory);
    copyBuffer(stagingBuffer, indexBuffer, bufferSize);
    device->destroyBuffer(stagingBuffer);
    device->freeMemory(stagingBufferMemory);
}

void VulkanApp::createUniformBuffers() {
    vk::DeviceSize bufferSize = sizeof(UniformBufferObject);
    uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        createBuffer(bufferSize, vk::BufferUsageFlagBits::eUniformBuffer,
                     vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                     uniformBuffers[i], uniformBuffersMemory[i]);
        uniformBuffersMapped[i] = device->mapMemory(uniformBuffersMemory[i], 0, bufferSize);
    }
}

void VulkanApp::createDescriptorPool() {
    vk::DescriptorPoolSize poolSize(
            vk::DescriptorType::eUniformBuffer,            // type
            static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) // descriptorCount
    );
    vk::DescriptorPoolCreateInfo poolInfo(
            {},                                             // flags
            static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT), // maxSets
            1,                                             // poolSizeCount
            &poolSize                                     // pPoolSizes
    );

    descriptorPool = device->createDescriptorPool(poolInfo);
}

void VulkanApp::createDescriptorSets() {
    std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
    vk::DescriptorSetAllocateInfo allocInfo(
            descriptorPool,                                 // descriptorPool
            static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT), // descriptor set count
            layouts.data()                                 // psetlayouts
    );

    descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    descriptorSets = device->allocateDescriptorSets(allocInfo);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vk::DescriptorBufferInfo bufferInfo(
                uniformBuffers[i],            // buffer
                0,                            // offset
                sizeof(UniformBufferObject) // range
        );

        vk::WriteDescriptorSet descriptorWrite;
        descriptorWrite.dstSet = descriptorSets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        device->updateDescriptorSets(descriptorWrite, nullptr);
    }
}

void VulkanApp::updateUniformBuffer(uint32_t currentImage) {
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), swapchainExtent.width / (float) swapchainExtent.height, 0.1f,
                                10.0f);
    ubo.proj[1][1] *= -1; // since glm was designed for opengl where y-up is positive whereas in vulkan y-up is negative

    memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

uint32_t VulkanApp::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
    vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("failed to find suitable memory type");
}

void VulkanApp::createCommandBuffers() {
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    vk::CommandBufferAllocateInfo allocInfo(
            commandPool,                      // commandPool
            vk::CommandBufferLevel::ePrimary, // level
            (uint32_t) commandBuffers.size()      // commandbuffercount
    );
    commandBuffers = device->allocateCommandBuffers(allocInfo);
}

void VulkanApp::recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex) {
    vk::CommandBufferBeginInfo beginInfo;
    commandBuffer.begin(beginInfo);

    std::array<float, 4> color = {0.0f, 0.0f, 0.0f, 1.0f};
    vk::ClearValue clearColor;
    clearColor.color = color;

    vk::RenderPassBeginInfo renderPassInfo(
            renderPass,                           // renderPass
            swapchainFramebuffers[imageIndex], // framebuffer
            {
                    // renderArea
                    {0, 0},            // renderArea.offset
                    swapchainExtent // renderArea.extent
            },
            1,            // clearValueCount
            &clearColor // pclearValues
    );

    commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);

    vk::Buffer vertexBuffers[] = {vertexBuffer};
    vk::DeviceSize offsets[] = {0};
    commandBuffer.bindVertexBuffers(0, vertexBuffers, offsets);
    commandBuffer.bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint16);

    vk::Viewport viewport(
            0, 0, (float) swapchainExtent.width, (float) swapchainExtent.height, 0.0f, 1.0f
            // x, y, width, height, minDepth, maxDepth
    );
    commandBuffer.setViewport(0, viewport);
    vk::Rect2D scissor(
            {0, 0}, swapchainExtent // offset, extent
    );
    commandBuffer.setScissor(0, scissor);

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, descriptorSets[currentFrame],
                                     nullptr);
    commandBuffer.drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

    commandBuffer.endRenderPass();
    commandBuffer.end();
}

void VulkanApp::createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    vk::SemaphoreCreateInfo semaphoreInfo;
    vk::FenceCreateInfo fenceInfo(vk::FenceCreateFlagBits::eSignaled);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        imageAvailableSemaphores[i] = device->createSemaphore(semaphoreInfo);
        renderFinishedSemaphores[i] = device->createSemaphore(semaphoreInfo);
        inFlightFences[i] = device->createFence(fenceInfo);
    }
}

void VulkanApp::drawFrame() {
    device->waitForFences(inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    try {
        auto result = device->acquireNextImageKHR(swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame]);

        if (result.result != vk::Result::eSuccess && result.result != vk::Result::eSuboptimalKHR) {
            throw std::runtime_error("failed to acquire swapchain image");
        }

        imageIndex = result.value;
    }
    catch (vk::OutOfDateKHRError err) {
        recreateSwapchain();
        return;
    }

    updateUniformBuffer(currentFrame);

    // Only reset the fence if we are submitting work
    device->resetFences(inFlightFences[currentFrame]);

    commandBuffers[currentFrame].reset();
    recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

    vk::SubmitInfo submitInfo;
    vk::Semaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

    vk::Semaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    graphicsQueue.submit(submitInfo, inFlightFences[currentFrame]);

    vk::SwapchainKHR swapchains[] = {swapchain};
    // waitSemaphoreCount, pWaitSemaphores, swapchaincount, pSwapchains, pImageIndices
    vk::PresentInfoKHR presentInfo(1, signalSemaphores, 1, swapchains, &imageIndex);

    try {
        auto presentResult = presentQueue.presentKHR(presentInfo);
        if (framebufferResized || presentResult == vk::Result::eSuboptimalKHR) {
            framebufferResized = false;
            recreateSwapchain();
        }
    }
    catch (vk::OutOfDateKHRError err) {
        framebufferResized = false;
        recreateSwapchain();
    }
    catch (...) {
        throw std::runtime_error("failed to present swapchain image");
    }
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

vk::ShaderModule VulkanApp::createShaderModule(const std::vector<char> &code) {
    vk::ShaderModuleCreateInfo createInfo(
            {},                                                // flags
            code.size(),                                    // code size
            reinterpret_cast<const uint32_t *>(code.data()) // pCode
    );

    return device->createShaderModule(createInfo);
}

vk::SurfaceFormatKHR
VulkanApp::chooseSwapchainSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &availableFormats) {
    for (const auto &format: availableFormats) {
        if (format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return format;
        }
    }
    // maybe rate the available formats and color spaces
    // in this case we'll just return the first one that's available
    return availableFormats[0];
}

vk::PresentModeKHR VulkanApp::chooseSwapchainPresentMode(const std::vector<vk::PresentModeKHR> &availablePresentModes) {
    for (const auto &presentMode: availablePresentModes) {
        if (presentMode == vk::PresentModeKHR::eMailbox) {
            return presentMode;
        }
    }
    return vk::PresentModeKHR::eFifo; // this is the only one guaranteed by spec
}

vk::Extent2D VulkanApp::chooseSwapExtend(const vk::SurfaceCapabilitiesKHR &capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)};

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                                        capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                                         capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

SwapchainSupportDetails VulkanApp::querySwapchainSupport(vk::PhysicalDevice device) {
    SwapchainSupportDetails details;
    details.capabilities = device.getSurfaceCapabilitiesKHR(surface);
    details.formats = device.getSurfaceFormatsKHR(surface);
    details.presentModes = device.getSurfacePresentModesKHR(surface);

    return details;
}

int VulkanApp::rateDeviceSuitability(vk::PhysicalDevice device) {
    QueueFamilyIndices indices = findQueueFamilies(device);
    bool extensionsSupported = checkDeviceExtensionSupport(device);
    vk::PhysicalDeviceProperties deviceProperties = device.getProperties();
    vk::PhysicalDeviceFeatures deviceFeatures = device.getFeatures();

    int score = 0;
    if (deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
        score += 1000;
    }

    // we could increase the score based on the gpu properties
    // score += deviceProperties.limits.maxImageDimension2D;

    bool swapchainAdequate = false;
    if (extensionsSupported) {
        SwapchainSupportDetails swapchainSupport = querySwapchainSupport(device);
        swapchainAdequate = !swapchainSupport.formats.empty() && !swapchainSupport.presentModes.empty();
    }

    if (!(indices.isComplete() && extensionsSupported && swapchainAdequate)) {
        score = 0;
    }
    std::cout << deviceProperties.deviceName << ": " << score << std::endl;
    return score;
}

bool VulkanApp::checkDeviceExtensionSupport(vk::PhysicalDevice device) {
    auto availableExtensions = device.enumerateDeviceExtensionProperties();

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto &extension: availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

QueueFamilyIndices VulkanApp::findQueueFamilies(vk::PhysicalDevice device) {
    QueueFamilyIndices indices;
    auto queueFamilies = device.getQueueFamilyProperties();

    int i = 0;
    VkBool32 presentSupport = false;
    for (const auto &queueFamily: queueFamilies) {
        if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
            indices.graphicsFamily = i;
        }
        presentSupport = device.getSurfaceSupportKHR(i, surface);
        if (presentSupport) {
            indices.presentFamily = i;
        }

        if (indices.isComplete())
            break;

        ++i;
    }

    return indices;
}

std::vector<const char *> VulkanApp::getRequiredExtensions() {
    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

bool VulkanApp::checkValidationLayerSupport() {
    auto availableLayers = vk::enumerateInstanceLayerProperties();

    for (const char *layerName: validationLayers) {
        bool layerFound = false;
        for (const auto &layerProperties: availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

// could only get it to work with original C API, no C++ bindings
VKAPI_ATTR VkBool32 VKAPI_CALL VulkanApp::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                        VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                        void *pUserData) {
    std::string severity;
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
        severity = "VERBOSE";
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        severity = "WARNING";
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
        severity = "INFO";
    else
        severity = "ERROR";

    std::cerr << "[" << severity << "]"
              << " validation layer : " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

void VulkanApp::DestroyDebugUtilsMessengerEXT(const VkAllocationCallbacks *pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(*instance,
                                                                            "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(*instance, debugMessenger, pAllocator);
    }
}
