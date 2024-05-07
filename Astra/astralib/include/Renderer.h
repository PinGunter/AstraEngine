#pragma once
#include <Pipeline.h>
#include <Scene.h>

namespace Astra {
	class Renderer {
	public:
		void render(const Scene & scene, Pipeline* pipeline);
	};
}