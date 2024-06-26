#pragma once
#include <Pipeline.h>

namespace Astra {
	template<typename T>
	struct RenderContext {
		Pipeline* pipeline;
		T& pushConstant;
		const CommandList& cmdList;
		uint32_t shaderStages;

		RenderContext(Pipeline* pl, T& pc, const CommandList& cl, uint32_t ss) : pipeline(pl), pushConstant(pc), cmdList(cl), shaderStages(ss) {}

		void pushConstants() {
			pipeline->pushConstants(cmdList, shaderStages, sizeof(T), &pushConstant);
		}
	};
}