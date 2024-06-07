#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <vulkan/vulkan.h>
#include <host_device.h>

/*
	vector<ObjDesc*> plano;

	añadirNodo()
		for nodo en escena{
			plano.add(nodo); // transfomraion
			nodo.children.transform = nodo.transform * nodo.children.tranform;
		}
	
	*/

namespace Astra {

	/**
	* Base Class for every 3d "thing" in the scene, everything in the scene derives this class
	* Contains its own transform matrix and a list of child nodes that also apply this matrix
	*/
	class Node3D {
	protected:
		static uint32_t NodeCount;
		glm::mat4 _transform;
		std::vector<Node3D*> _children;
		std::string _name;
		uint32_t _id;

	public:
		Node3D(const glm::mat4& transform = glm::mat4(1.0f), const std::string& name = "");
		virtual void destroy() {}

		bool operator==(const Node3D& other);

		void addChild(Node3D* child);
		void removeChild(const Node3D& child);

		// TRANSFORM OPERATIONS

		void rotate(const glm::vec3& axis, const float& angle);

		void scale(const glm::vec3& scaling);

		void translate(const glm::vec3& position);

		// GETTERS
		glm::vec3 getPosition() const;
		glm::vec3 getRotation() const;
		glm::vec3 getScale() const;

		glm::mat4& getTransformRef() { return _transform; }

		const glm::mat4& getTransform() const { return _transform; }

		std::vector<Node3D*>& getChildren() { return _children; }

		std::string& getNameRef();
		std::string getName() const;

		uint32_t getID() const;

		// SETTERS
		void setName(const std::string& n) { _name = n; }

		/**
		* will be called every frame in Astra::App
		*
		*/
		virtual void update() = 0;

		virtual void updatePushConstantRaster(PushConstantRaster& pc) const = 0;
		virtual void updatePushConstantRT(PushConstantRay& pc)const = 0;

	};
}