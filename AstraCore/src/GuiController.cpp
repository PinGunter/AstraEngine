#include <GuiController.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <vector>
#include <Device.h>
#include <glm/gtc/type_ptr.hpp>


void Astra::GuiController::init(GLFWwindow* window, Astra::Renderer* renderer)
{
	VkRenderPass renderpass;
	VkFormat color, depth;
	int imageCount;
	renderer->getGuiControllerInfo(renderpass, imageCount, color, depth);

	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	//io.IniFilename = nullptr;  // Avoiding the INI file
	io.LogFilename = nullptr;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // Enable Docking

	//ImGuiH::setStyle();
	//ImGuiH::setFonts();

	std::vector<VkDescriptorPoolSize> poolSize{ {VK_DESCRIPTOR_TYPE_SAMPLER, 1}, {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1} };
	VkDescriptorPoolCreateInfo poolInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolInfo.maxSets = 1000;
	poolInfo.poolSizeCount = 2;
	poolInfo.pPoolSizes = poolSize.data();
	vkCreateDescriptorPool(AstraDevice.getVkDevice(), &poolInfo, nullptr, &_imguiDescPool);

	// Setup Platform/Renderer back ends
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = AstraDevice.getVkInstance();
	init_info.PhysicalDevice = AstraDevice.getPhysicalDevice();
	init_info.Device = AstraDevice.getVkDevice();
	init_info.QueueFamily = AstraDevice.getGraphicsQueueIndex();
	init_info.Queue = AstraDevice.getQueue();
	init_info.PipelineCache = VK_NULL_HANDLE;
	init_info.DescriptorPool = _imguiDescPool;
	init_info.RenderPass = renderpass;
	init_info.Subpass = 0;
	init_info.MinImageCount = 2;
	init_info.ImageCount = imageCount;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;  // <--- need argument?
	init_info.CheckVkResultFn = nullptr;
	init_info.Allocator = nullptr;

	init_info.UseDynamicRendering = false;
	init_info.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
	init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
	init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &color;
	init_info.PipelineRenderingCreateInfo.depthAttachmentFormat = depth;

	ImGui_ImplVulkan_Init(&init_info);

	// Upload Fonts
	ImGui_ImplVulkan_CreateFontsTexture();

	// link with glfw
	ImGui_ImplGlfw_InitForVulkan(window, true);
}

void Astra::GuiController::startFrame()
{
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGuizmo::BeginFrame();
}

void Astra::GuiController::endFrame(const Astra::CommandList& cmdList)
{
	ImGui::Render();
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdList.getCommandBuffer());
}

void Astra::GuiController::destroy()
{
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	vkDestroyDescriptorPool(AstraDevice.getVkDevice(), _imguiDescPool, nullptr);
}

