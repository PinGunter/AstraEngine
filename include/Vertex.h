#pragma once
#include <vulkan/vulkan.hpp>
#include <array>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/glm.hpp>
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

	bool operator==(const Vertex& other) const {
		return pos == other.pos && color == other.color && texCoord == other.texCoord;
	}
};
namespace std {
	template<> struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
}