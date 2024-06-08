/*
 * Copyright (c) 2014-2021, NVIDIA CORPORATION.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-FileCopyrightText: Copyright (c) 2014-2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */


 // ImGui - standalone example application for Glfw + Vulkan, using programmable
 // pipeline If you are new to ImGui, see examples/README.txt and documentation
 // at the top of imgui.cpp.

#include <array>

#define IMGUI_DEFINE_MATH_OPERATORS
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#include "imgui.h"
#include "imgui/imgui_helper.h"

#include "hello_vulkan.h"
#include "nvh/fileoperations.hpp"
#include "nvpsystem.hpp"
#include "nvvk/commands_vk.hpp"
#include "nvvk/context_vk.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <imgui/ImGuizmo.h>
#include <iostream>
#include <Device.h>
#include <Utils.h>

#include "nvvk/shaders_vk.hpp"

//////////////////////////////////////////////////////////////////////////
#define UNUSED(x) (void)(x)
//////////////////////////////////////////////////////////////////////////

// Default search path for shaders
std::vector<std::string> defaultSearchPaths;


// GLFW Callback functions
static void onErrorCallback(int error, const char* description)
{
	fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

// Extra UI
void renderUI(HelloVulkan& helloVk)
{
	//ImGuiH::CameraWidget();
	if (ImGui::CollapsingHeader("Light"))
	{
		ImGui::RadioButton("Point", &helloVk.m_pcRaster.lightType, 0);
		ImGui::SameLine();
		ImGui::RadioButton("Infinite", &helloVk.m_pcRaster.lightType, 1);

		ImGui::SliderFloat3("Position", &helloVk.m_pcRaster.lightPosition.x, -20.f, 20.f);
		ImGui::SliderFloat("Intensity", &helloVk.m_pcRaster.lightIntensity, 0.f, 150.f);
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
static int const SAMPLE_WIDTH = 1280;
static int const SAMPLE_HEIGHT = 720;


//--------------------------------------------------------------------------------------------------
// Application Entry
//
int main2(int argc, char** argv)
{
	UNUSED(argc);

	// Setup GLFW window
	glfwSetErrorCallback(onErrorCallback);
	if (!glfwInit())
	{
		return 1;
	}
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow* window = glfwCreateWindow(SAMPLE_WIDTH, SAMPLE_HEIGHT, PROJECT_NAME, nullptr, nullptr);


	Astra::Camera cam;
	Astra::CameraController* camera = new Astra::OrbitCameraController(cam);
	// Setup camera
	camera->setWindowSize(SAMPLE_WIDTH, SAMPLE_HEIGHT);
	camera->setLookAt(glm::vec3(5.0f), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

	// Setup Vulkan
	if (!glfwVulkanSupported())
	{
		printf("GLFW: Vulkan Not Supported\n");
		return 1;
	}

	// setup some basic things for the sample, logging file for example
	NVPSystem system(PROJECT_NAME);

	// Search path for shaders and other media
	defaultSearchPaths = {
		NVPSystem::exePath() + PROJECT_RELDIRECTORY,
		NVPSystem::exePath() + PROJECT_RELDIRECTORY "..",
		std::string(PROJECT_NAME),
	};

	// Vulkan required extensions
	assert(glfwVulkanSupported() == 1);
	uint32_t count{ 0 };
	auto     reqExtensions = glfwGetRequiredInstanceExtensions(&count);

	// Requesting Vulkan extensions and layers
	nvvk::ContextCreateInfo contextInfo;
	contextInfo.setVersion(1, 2);                       // Using Vulkan 1.2
	for (uint32_t ext_id = 0; ext_id < count; ext_id++)  // Adding required extensions (surface, win32, linux, ..)
		contextInfo.addInstanceExtension(reqExtensions[ext_id]);
	contextInfo.addInstanceLayer("VK_LAYER_LUNARG_monitor", true);              // FPS in titlebar
	contextInfo.addInstanceExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, true);  // Allow debug names
	contextInfo.addDeviceExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);            // Enabling ability to present rendering
	contextInfo.addDeviceExtension(VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME); // enable shader printf;

	// add vulkan raytracing extensions
	VkPhysicalDeviceAccelerationStructureFeaturesKHR accelFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR };
	contextInfo.addDeviceExtension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, false, &accelFeatures); // to build acceleration structures
	VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtPipelineFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR };
	contextInfo.addDeviceExtension(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, false, &rtPipelineFeatures); // raytracing pipeline
	contextInfo.addDeviceExtension(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME); // required by raytracing pipeline

	//Astra::DeviceCreateInfo dci{};
	//Astra::Device::getInstance().initDevice(dci);

	// Creating Vulkan base application
	nvvk::Context vkctx{};
	vkctx.initInstance(contextInfo);
	// Find all compatible devices
	auto compatibleDevices = vkctx.getCompatibleDevices(contextInfo);
	assert(!compatibleDevices.empty());
	// Use a compatible device
	vkctx.initDevice(compatibleDevices[0], contextInfo);

	// Create example
	HelloVulkan helloVk;
	helloVk.setCamera(camera);

	// Window need to be opened to get the surface on which to draw
	const VkSurfaceKHR surface = helloVk.getVkSurface(vkctx.m_instance, window);
	vkctx.setGCTQueueWithPresent(surface);

	helloVk.setup(vkctx.m_instance, vkctx.m_device, vkctx.m_physicalDevice, vkctx.m_queueGCT.familyIndex);
	helloVk.createSwapchain(surface, SAMPLE_WIDTH, SAMPLE_HEIGHT);
	helloVk.createDepthBuffer();
	helloVk.createRenderPass();
	helloVk.createFrameBuffers();

	// Setup Imgui
	helloVk.initGUI(0);  // Using sub-pass 0

	// Creation of the example
	helloVk.loadModel(nvh::findFile("media/scenes/cube_multi.obj", defaultSearchPaths, true));
	helloVk.loadModel(nvh::findFile("media/scenes/mono.obj", defaultSearchPaths, true));
	helloVk.loadModel(nvh::findFile("media/scenes/Medieval_Building.obj", defaultSearchPaths, true));
	helloVk.loadModel(nvh::findFile("media/scenes/plane.obj", defaultSearchPaths, true));
	helloVk.loadModel(nvh::findFile("media/scenes/vaca.obj", defaultSearchPaths, true));

	auto& instances = helloVk.getInstances();

	std::vector<const char*> names;
	for (auto& i : instances) {
		names.push_back(i.getNameRef().c_str());
	}

	int currentModel = 0;

	helloVk.createOffscreenRender();
	helloVk.createDescriptorSetLayout(); // app
	helloVk.createGraphicsPipeline(); // app
	helloVk.createUniformBuffer(); // app
	helloVk.createObjDescriptionBuffer(); // app
	helloVk.updateDescriptorSet(); // app

	helloVk.initRayTracing();
	helloVk.createBottomLevelAS();
	helloVk.createTopLevelAS();
	helloVk.createRtDescriptorSet();
	helloVk.createRtPipeline();
	helloVk.createRtShaderBindingTable();

	helloVk.createPostDescriptor();
	helloVk.createPostPipeline();
	helloVk.updatePostDescriptorSet();
	glm::vec4 clearColor = glm::vec4(1, 1, 1, 1.00f);


	helloVk.setupGlfwCallbacks(window);
	ImGui_ImplGlfw_InitForVulkan(window, true);

	bool useRaytracer = false;
	bool showGuizmo = false;
	int guizmo_type = ImGuizmo::UNIVERSAL;
	bool text_edit = false;
	std::string text_edit_str;
	// Main loop
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		if (helloVk.isMinimized())
			continue;

		// Start the Dear ImGui frame
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGuizmo::BeginFrame();


		// Show UI window.
		if (helloVk.showGui())
		{
			ImGuiH::Panel::Begin();


			if (ImGui::BeginTabBar("Principal")) {
				if (ImGui::BeginTabItem("Rendering")) {
					ImGui::ColorEdit3("Clear color", reinterpret_cast<float*>(&clearColor));
					ImGui::Checkbox("Use Raytracer", &useRaytracer);
					renderUI(helloVk);
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Performance")) {
					ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
					ImGui::EndTabItem();
				}


				ImGui::EndTabBar();

			}

			ImGuiH::Panel::End();

			ImGui::Text("Models: ");
			ImGui::ListBox("###Models", &currentModel, names.data(), names.size());

			if (ImGui::Button("REBUILD TLAS")) helloVk.updateTLAS(currentModel);
			if (ImGui::Checkbox("Visible: ", &instances[currentModel].getVisibleRef())) helloVk.updateTLAS(currentModel);
			ImGui::Checkbox("ShowGuizmo: ", &showGuizmo);

			for (int i = 0; i < 4; i++) {
				for (int j = 0; j < 4; j++) {
					ImGui::Text(std::to_string(instances[currentModel].getTransformRef()[j][i]).c_str()); ImGui::SameLine();
				}
				ImGui::Spacing();
			}

			ImGui::Text("Guizmo Type:");
			ImGui::RadioButton("Universal", &guizmo_type, (int)ImGuizmo::UNIVERSAL);
			ImGui::RadioButton("Translate", &guizmo_type, (int)ImGuizmo::TRANSLATE);
			ImGui::RadioButton("Rotate", &guizmo_type, (int)ImGuizmo::ROTATE);
			ImGui::RadioButton("Scale", &guizmo_type, (int)ImGuizmo::SCALE);

			ImGuiIO& io = ImGui::GetIO();
			ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
			glm::mat4      proj = camera->getProjectionMatrix();
			proj[1][1] *= -1;
			if (showGuizmo) ImGuizmo::Manipulate(glm::value_ptr(camera->getViewMatrix()), glm::value_ptr(proj), static_cast<ImGuizmo::OPERATION>(guizmo_type), ImGuizmo::LOCAL, glm::value_ptr(instances[currentModel].getTransformRef()));

			if (ImGuizmo::IsUsing()) {
				helloVk.updateTLAS(currentModel);
			}



			ImGui::ShowDemoWindow();
		}

		// Start rendering the scene
		helloVk.prepareFrame();

		// Start command buffer of this frame
		auto                   curFrame = helloVk.getCurFrame();
		const VkCommandBuffer& cmdBuf = helloVk.getCommandBuffers()[curFrame];

		VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(cmdBuf, &beginInfo);

		// Updating camera buffer
		helloVk.updateUniformBuffer(cmdBuf);

		// Clearing screen
		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { {clearColor[0], clearColor[1], clearColor[2], clearColor[3]} };
		clearValues[1].depthStencil = { 1.0f, 0 };

		// Offscreen render pass
		{
			VkRenderPassBeginInfo offscreenRenderPassBeginInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
			offscreenRenderPassBeginInfo.clearValueCount = 2;
			offscreenRenderPassBeginInfo.pClearValues = clearValues.data();
			offscreenRenderPassBeginInfo.renderPass = helloVk.m_offscreenRenderPass;
			offscreenRenderPassBeginInfo.framebuffer = helloVk.m_offscreenFramebuffer;
			offscreenRenderPassBeginInfo.renderArea = { {0, 0}, helloVk.getSize() };

			// Rendering Scene
			if (useRaytracer) {
				helloVk.raytrace(cmdBuf, clearColor);
			}
			else {
				vkCmdBeginRenderPass(cmdBuf, &offscreenRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
				helloVk.rasterize(cmdBuf);
				vkCmdEndRenderPass(cmdBuf);
			}
		}


		// 2nd rendering pass: tone mapper, UI
		{
			VkRenderPassBeginInfo postRenderPassBeginInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
			postRenderPassBeginInfo.clearValueCount = 2;
			postRenderPassBeginInfo.pClearValues = clearValues.data();
			postRenderPassBeginInfo.renderPass = helloVk.getRenderPass();
			postRenderPassBeginInfo.framebuffer = helloVk.getFramebuffers()[curFrame];
			postRenderPassBeginInfo.renderArea = { {0, 0}, helloVk.getSize() };

			// Rendering tonemapper
			vkCmdBeginRenderPass(cmdBuf, &postRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
			helloVk.drawPost(cmdBuf);
			// Rendering UI
			ImGui::Render();
			ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuf);
			vkCmdEndRenderPass(cmdBuf);
		}

		// Submit for display
		vkEndCommandBuffer(cmdBuf);
		helloVk.submitFrame();
	}

	// Cleanup
	vkDeviceWaitIdle(helloVk.getDevice());

	helloVk.destroyResources();
	helloVk.destroy();
	vkctx.deinit();

	glfwDestroyWindow(window);
	glfwTerminate();

	delete camera;
	return 0;
}
