#include <Mesh.h>
#include <Device.h>

Astra::MeshInstance::MeshInstance(uint32_t mesh, const glm::mat4& transform, const std::string& name) : Node3D(transform, name), _mesh(mesh)
{
}

Astra::MeshInstance& Astra::MeshInstance::operator=(const MeshInstance& other)
{
	_transform = other._transform;
	_children = other._children;
	_name = other._name;
	_id = other._id;
	_mesh = other._mesh;
	return *this;
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

void Astra::Mesh::draw(const CommandList& cmdList) const
{
	cmdList.drawIndexed(vertexBuffer.buffer, indexBuffer.buffer, indices.size());
}

void Astra::Mesh::createBuffers(const Astra::CommandList& cmdList, nvvk::ResourceAllocatorDma* alloc) {
	const auto& cmdBuf = cmdList.getCommandBuffer();
	VkBufferUsageFlags flag = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	VkBufferUsageFlags rayTracingFlags = flag | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	vertexBuffer = alloc->createBuffer(cmdBuf, vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | rayTracingFlags);
	indexBuffer = alloc->createBuffer(cmdBuf, indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | rayTracingFlags);
	matColorBuffer = alloc->createBuffer(cmdBuf, materials, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | rayTracingFlags);
	matIndexBuffer = alloc->createBuffer(cmdBuf, materialIndices, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | rayTracingFlags);
}
