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
	//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;    // Enable Multi-Viewport / Platform Windows

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


void Astra::BasiGui::draw(App* app)
{
	DefaultApp* dapp = (DefaultApp*)app;
	DefaultSceneRT* scene = (DefaultSceneRT*)dapp->getCurrentScene();

	ImGui::Begin("Inspector");

	if (ImGui::BeginTabBar("###Tabbar")) {
		if (ImGui::BeginTabItem("Renderer")) {
			ImGui::ColorEdit3("Clear Color", glm::value_ptr(app->getRenderer()->getClearColorRef()));

			ImGui::SliderInt("Max Ray Recursion Depth", &app->getRenderer()->getMaxDepthRef(), 0, AstraDevice.getRtProperties().maxRayRecursionDepth - 1);


			ImGui::Separator();

			ImGui::Text("Select rendering pipeline");
			ImGui::RadioButton("RayTracing", &dapp->getSelectedPipelineRef(), 0);
			ImGui::RadioButton("Raster", &dapp->getSelectedPipelineRef(), 1);
			ImGui::RadioButton("Wireframe", &dapp->getSelectedPipelineRef(), 2);

			ImGui::EndTabItem();

		}
		if (ImGui::BeginTabItem("Performance")) {
			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}

	ImGui::End();


	ImGui::Begin("Scene");

	ImGui::Text((std::string("Current Scene ") + std::to_string(app->getCurrentSceneIndex())).c_str());
	ImGui::SameLine();
	if (ImGui::Button("Switch")) {
		app->setCurrentSceneIndex((app->getCurrentSceneIndex() + 1) % 2);
		_node = 0;
	}

	if (ImGui::BeginListBox("Instances")) {

		for (int i = 0; i < scene->getInstances().size(); i++) {
			auto& inst = scene->getInstances()[i];
			if (ImGui::Selectable(inst.getName().c_str(), i == _node)) {
				_node = i;
				_handlingNodes = true;
			}
		}
		ImGui::EndListBox();
	}

	if (ImGui::BeginListBox("Lights")) {

		for (int i = 0; i < scene->getLights().size(); i++) {
			auto light = scene->getLights()[i];
			if (ImGui::Selectable(light->getName().c_str(), i == _node)) {
				_node = i;
				_handlingNodes = false;
			}
		}
		ImGui::EndListBox();
	}
	if (ImGui::Checkbox("Visible", &scene->getInstances()[_node].getVisibleRef())) {
		scene->updateTopLevelAS(_node);
	}

	if (ImGui::Button("New instance")) {
		dapp->addInstanceToScene(MeshInstance(scene->getInstances()[_node].getMeshIndex(), scene->getInstances()[_node].getTransform(), scene->getInstances()[_node].getName() + " copy" + std::to_string(_ncopies++)));
	}

	ImGuiIO& io = ImGui::GetIO();
	ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
	glm::mat4      proj = scene->getCamera()->getProjectionMatrix();
	proj[1][1] *= -1;

	if (ImGui::IsKeyPressed(ImGuiKey_Space)) {
		_showGuizmo = !_showGuizmo;
	}

	if (_showGuizmo)
		if (_handlingNodes) {
			if (ImGuizmo::Manipulate(glm::value_ptr(scene->getCamera()->getViewMatrix()),
				glm::value_ptr(proj),
				ImGuizmo::OPERATION::UNIVERSAL,
				ImGuizmo::LOCAL,
				glm::value_ptr(app->getCurrentScene()->getInstances()[_node].getTransformRef()))) {
				scene->updateTopLevelAS(_node);
			}
		}
		else {
			if (ImGuizmo::Manipulate(glm::value_ptr(scene->getCamera()->getViewMatrix()),
				glm::value_ptr(proj),
				ImGuizmo::OPERATION::UNIVERSAL,
				ImGuizmo::LOCAL,
				glm::value_ptr(app->getCurrentScene()->getLights()[_node]->getTransformRef()))) {
				scene->updateTopLevelAS(_node);
			}
		}
	ImGui::End();

}
