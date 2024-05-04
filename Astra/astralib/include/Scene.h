#pragma once
#include <Node3D.h>
#include <vulkan/vulkan.hpp>

namespace Astra {
	class Scene {
	private:
		std::vector<Node3D * > _nodes;
		vk::AccelerationStructureKHR _tlas;
		glm::mat4 _transform;
	public:
		void draw(/*render pipeline*/);
		void addNode(Node3D *n);
		void removeNode(const Node3D& n);
	};
}