#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <string>

namespace Astra {

	/**
	* Base Class for every 3d "thing" in the scene, everything in the scene derives this class
	* Contains its own transform matrix and a list of child nodes that also apply this matrix
	*/
	class Node3D {
	protected:
		static uint32_t n_nodes;
		glm::mat4 _transform;
		std::vector<Node3D*> _children;
		std::string _name;

	public:

		Node3D(const glm::mat4& transform, const std::string& name = "");

		// TRANSFORM OPERATIONS

		void rotate(const glm::vec3& axis, const float& angle);

		void scale(const glm::vec3& scaling);

		void translate(const glm::vec3& position);

		// GETTERS
		glm::mat4& getTransform() { return _transform; }

		const glm::mat4& getTransform() const { return _transform; }

		std::vector<Node3D*>& getChildren() { return _children; }

		std::string& getName();
		std::string getName() const;

		// SETTERS
		void setName(const std::string& n) { _name = n; }

		/**
		* will be called every frame in Astra::App
		*
		*/

		// TODO: probably needs pipeline or render reference
		virtual void update() {};
	};
}