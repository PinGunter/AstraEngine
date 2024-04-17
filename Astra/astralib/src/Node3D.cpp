#include <Node3D.h>
#include <random>
#include <glm/gtc/matrix_transform.hpp>

using namespace Astra;

uint32_t Node3D::_n_nodes = 0;

Node3D::Node3D(const glm::mat4& transform_mat, const std::string& name) : _transform(transform_mat), _name(name) {
	if (name == "") {
		_name = std::string("Node3D - ") + std::to_string(Node3D::_n_nodes++);
	}
}

void Node3D::rotate(const glm::vec3& axis, const float& angle) {
	_transform = glm::rotate(_transform, angle, axis);
}

void Node3D::scale(const glm::vec3& scaling) {
	_transform = glm::scale(_transform, scaling);
}

void Node3D::translate(const glm::vec3& position) {
	_transform = glm::translate(_transform, position);
}

std::string& Astra::Node3D::getName()
{
	return _name;
}

std::string Astra::Node3D::getName() const
{
	return _name;
}
