#pragma once
#include <Pipeline.h>

namespace Astra
{
	/**
	 * \~spanish @brief Struct con plantilla para admitir los distintos tipos de push constant. Contiene la información necesaria para pasar a la escena.
	 * El objetivo de esta estructura es abstraer la llamada a pushConstants desde fuera del renderer
	 * \~english @brief Template struct to support different push constant types. Contains the rendering information for the scene.
	 * The main goal of this struct is to wrap the pushConstants call.
	 */
	template <typename T>
	struct RenderContext
	{
		Pipeline *pipeline;
		T &pushConstant;
		const CommandList &cmdList;
		uint32_t shaderStages;

		RenderContext(Pipeline *pl, T &pc, const CommandList &cl, uint32_t ss) : pipeline(pl), pushConstant(pc), cmdList(cl), shaderStages(ss) {}

		void pushConstants()
		{
			pipeline->pushConstants(cmdList, shaderStages, sizeof(T), &pushConstant);
		}
	};
}