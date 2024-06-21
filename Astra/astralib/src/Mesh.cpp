#include <Mesh.h>
#include <Device.h>

Astra::MeshInstance::MeshInstance(uint32_t mesh_index, const glm::mat4& transform, const std::string& name) : Node3D(transform, name), _mesh(mesh_index)
{
}

void Astra::MeshInstance::setVisible(bool v)
{
	_visible = v;
}


bool Astra::MeshInstance::getVisible() const
{
	return _visible;
}

bool& Astra::MeshInstance::getVisibleRef()
{
	return _visible;
}

uint32_t Astra::MeshInstance::getMeshIndex() const
{
	return _mesh;
}

bool Astra::MeshInstance::update()
{
	return false;
}

void Astra::MeshInstance::destroy()
{
}

void Astra::MeshInstance::updatePushConstantRaster(PushConstantRaster& pc) const
{
	pc.modelMatrix = _transform;
	pc.objIndex = _mesh;
}

void Astra::MeshInstance::updatePushConstantRT(PushConstantRay& pc) const
{
	// nothing to do
}

void Astra::HostModel::draw(const Astra::CommandList& cmdList) const
{
	cmdList.drawIndexed(vertexBuffer.buffer, indexBuffer.buffer, nbIndices);
}
