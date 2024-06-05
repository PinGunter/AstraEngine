#include <Node3D.h>
#include <random>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

using namespace Astra;

uint32_t Node3D::NodeCount = 0;

Node3D::Node3D(const glm::mat4& transform_mat, const std::string& name) : _transform(transform_mat), _name(name) {
	_id = NodeCount++;

	if (name == "") {
		_name = std::string("Node3D - ") + std::to_string(Node3D::NodeCount);
	}
}

bool Astra::Node3D::operator==(const Node3D& other)
{
	return _id == other._id;
}

void Astra::Node3D::addChild(Node3D* child)
{
	_children.push_back(child);
}

void Astra::Node3D::removeChild(const Node3D& child)
{
	auto eraser = _children.begin();
	bool found = false;
	for (auto it = _children.begin(); it != _children.end() && !found; ++it) {
		if (*(*it) == child) {
			eraser = it;
			found = true;
		}
	}
	if (found)	_children.erase(eraser);
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

glm::vec3 Astra::Node3D::getPosition()
{
	glm::vec3 scale;
	glm::quat rotation;
	glm::vec3 translation;
	glm::vec3 skew;
	glm::vec4 perspective;

	glm::decompose(_transform, scale, rotation, translation, skew, perspective);

	return translation;
}

glm::vec3 Astra::Node3D::getRotation()
{
	glm::vec3 scale;
	glm::quat rotation;
	glm::vec3 translation;
	glm::vec3 skew;
	glm::vec4 perspective;

	glm::decompose(_transform, scale, rotation, translation, skew, perspective);

	return glm::eulerAngles(rotation);
}

glm::vec3 Astra::Node3D::getScale()
{
	glm::vec3 scale;
	glm::quat rotation;
	glm::vec3 translation;
	glm::vec3 skew;
	glm::vec4 perspective;

	glm::decompose(_transform, scale, rotation, translation, skew, perspective);

	return scale;
}

std::string& Astra::Node3D::getName()
{
	return _name;
}

std::string Astra::Node3D::getName() const
{
	return _name;
}

uint32_t Astra::Node3D::getID() const
{
	return _id;
}
