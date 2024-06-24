#pragma once
#include <App.h>

namespace Astra {
	class AppRT : virtual public App {
	protected:
		nvvk::DescriptorSetBindings _rtDescSetLayoutBind;
		VkDescriptorPool _rtDescPool;
		VkDescriptorSetLayout _rtDescSetLayout;
		VkDescriptorSet _rtDescSet;
		std::vector<nvvk::AccelKHR> _blas;
		std::vector<VkAccelerationStructureInstanceKHR> m_tlas;

		virtual void createRtDescriptorSet() = 0;
		virtual void updateRtDescriptorSet() = 0;
	};
}