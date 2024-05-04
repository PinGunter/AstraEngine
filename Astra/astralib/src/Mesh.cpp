#include <Mesh.h>

Astra::MeshInstance::MeshInstance(uint32_t mesh_index, const glm::mat4& transform, const std::string& name) : Node3D(transform, name), _mesh(mesh_index)
{
}

void Astra::MeshInstance::setVisible(bool v)
{
	this->_visible = v;
}

void Astra::MeshInstance::setBLAS(const vk::AccelerationStructureKHR & blas)
{
	_blas = blas;
}

bool Astra::MeshInstance::getVisible() const
{
	return _visible;
}

bool& Astra::MeshInstance::getVisible()
{
	return _visible;
}

vk::AccelerationStructureKHR Astra::MeshInstance::getBLAS() const
{
	return _blas;
}

uint32_t Astra::MeshInstance::getMeshIndex() const
{
	return _mesh;
}

void Astra::MeshInstance::update()
{
	// called for every frame
}
