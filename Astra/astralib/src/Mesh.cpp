#include <Mesh.h>

Astra::MeshInstance::MeshInstance(uint32_t mesh_index, const glm::mat4& transform, const std::string& name) : Node3D(transform, name), _mesh(mesh_index)
{
}

void Astra::MeshInstance::setBLAS(const VkAccelerationStructureKHR & blas)
{
	_blas = blas;
}

void Astra::MeshInstance::setVisible(bool v)
{
	_visible = v;
}

VkAccelerationStructureKHR Astra::MeshInstance::getBLAS() const
{
	return _blas;
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

void Astra::MeshInstance::update()
{
	// called for every frame
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

void Astra::HostModel::draw(const VkCommandBuffer& cmdBuf, VkDeviceSize offset) const
{
	vkCmdBindVertexBuffers(cmdBuf, 0, 1, &vertexBuffer.buffer, &offset);
	vkCmdBindIndexBuffer(cmdBuf, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(cmdBuf, nbIndices, 1, 0, 0, 0);
}
