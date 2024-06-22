#pragma once
#include <vulkan/vulkan.h>
namespace Astra {
	enum InputMods {
		Shift = 0x0001,
		Control = 0x0002,
		Alt = 0x0004
	};

	enum MouseButtons {
		Left = 0,
		Right = 1,
		Middle = 2
	};

	enum PipelineBindPoints {
		Graphics = VK_PIPELINE_BIND_POINT_GRAPHICS,
		Compute = VK_PIPELINE_BIND_POINT_COMPUTE,
		RayTracing = VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR
	};

	enum AppStatus {
		Created,
		Running,
		Destroyed
	};
}