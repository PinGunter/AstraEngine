#pragma once
#include <App.h>

namespace Astra
{
	/**
	 * \~spanish @class AppRT
	 * \~spanish @brief Clase que deriva de App para agregar los datos y métodos necesarios para usar trazado de rayos.
	 * \~english @class AppRT
	 * \~english @brief Class that derives from App and adds the necessary data and methods to use ray-tracing.
	 */
	class AppRT : public App
	{
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
		void resetScene(bool recreatePipelines) override;

	public:
		void init(const std::vector<Scene*>& scenes, Renderer* renderer, GuiController* gui = nullptr) override;
		void destroy() override;
	};

}