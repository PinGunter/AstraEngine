#pragma once
#include <Pipeline.h>
#include <Scene.h>

namespace Astra {
	class Renderer {
	protected:
		PostPipeline _postPipeline;
		VkRenderPass _postRenderPass;

		std::vector<Pipeline* > _pipelines; // in this case will use 0 for rt, 1 for raster

		void renderPost(); // mandatory step! after drawing
		void renderRaster(const Scene& scene, RasterPipeline* pipeline);
		void renderRaytrace(const Scene& scene, RayTracingPipeline* pipeline);
	public:
		void render(const Scene & scene, Pipeline* pipeline);
	};
}