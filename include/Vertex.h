#pragma once
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>
#include <array>
/**
* Struct to store vertex info
*/

struct Vertex {
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;

	static vk::VertexInputBindingDescription getBindingDescription() {
		return vk::VertexInputBindingDescription(
			0, // binding
			sizeof(Vertex), // stride
			vk::VertexInputRate::eVertex // inputRate
		);
	}

	static std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescriptions() {
		// arguments in order:
		// location, binding, format, offset
		return { {
			vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos)),
			vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color)),
			vk::VertexInputAttributeDescription(2, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, texCoord))
			} };
	}
};