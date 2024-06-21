#pragma once
#include <vulkan/vulkan.h>
namespace Astra {
	enum InputMods {
		SHIFT = 0x0001,
		CONTROL = 0x0002,
		ALT = 0x0004
	};

	enum MouseButtons {
		LEFT = 0,
		RIGHT = 1,
		MIDDLE = 2
	};

	enum PipelineBindPoints {
		Graphics = VK_PIPELINE_BIND_POINT_GRAPHICS,
		Compute = VK_PIPELINE_BIND_POINT_COMPUTE,
		RayTracing = VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR
	};
}