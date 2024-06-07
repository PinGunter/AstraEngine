#pragma once
#include <vulkan/vulkan.h>
#include <Renderer.h>

namespace Astra {
	class Gui {
		friend class Renderer;
	protected:
		VkDescriptorPool _imguiDescPool;
	public:
		void init();
		
	};
}