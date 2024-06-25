#pragma once
#include <App.h>

namespace Astra {
	class AppRT : public App {
	protected:
		nvvk::DescriptorSetBindings _rtDescSetLayoutBind;
		VkDescriptorPool _rtDescPool;
		VkDescriptorSetLayout _rtDescSetLayout;
		VkDescriptorSet _rtDescSet;
		std::vector<nvvk::AccelKHR> _blas;
		std::vector<VkAccelerationStructureInstanceKHR> m_tlas;

		virtual void createRtDescriptorSetLayout();
		virtual void updateRtDescriptorSet();
		void onResize(int w, int h) override;

	public:
		void init(const std::vector<Scene*>& scenes, Renderer* renderer, GuiController* gui = nullptr) override;
		void destroy() override;
	};

}