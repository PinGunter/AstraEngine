#pragma once
#include <Node3D.h>
#include <Light.h>
#include <Camera.h>
#include <vulkan/vulkan.h>

namespace Astra {
	class Scene {
	private:
		std::vector<Node3D* > _nodes;
		Light* _light; // multiple lights in the future
		CameraController* _camera;
		VkAccelerationStructureKHR _tlas;
		glm::mat4 _transform;
	public:
		Scene();
		void addNode(Node3D *n);
		void removeNode(const Node3D& n);
		void addLight(Light* l);
		void setCamera(CameraController* c);
		void setTLAS(VkAccelerationStructureKHR tlas);

		Light* getLight() const;
		CameraController* getCamera() const;
		VkAccelerationStructureKHR getTLAS() const;
		
		glm::mat4& getTransform();
	};
}