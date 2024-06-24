#pragma once
#include <App.h>

namespace Astra {
	class AppRaster : virtual public App {
	protected:
		nvvk::DescriptorSetBindings _descSetLayoutBind;
		VkDescriptorPool _descPool;
		VkDescriptorSetLayout _descSetLayout;
		VkDescriptorSet _descSet;

		virtual void createDescriptorSetLayout() = 0;
		virtual void updateDescriptorSet() = 0;
	};
}