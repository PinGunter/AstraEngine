#include <Mesh.h>
#include <Device.h>
#include <nvvk/buffers_vk.hpp>
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <Utils.h>

Astra::MeshInstance::MeshInstance(uint32_t mesh, const glm::mat4& transform, const std::string& name) : Node3D(transform, name), _mesh(mesh)
{
	if (name.empty()) {
		_name = "Mesh - " + std::to_string(_id);
	}
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

void Astra::Mesh::create(const Astra::CommandList& cmdList, nvvk::ResourceAllocatorDma* alloc, uint32_t txtOffset)
{
	assert(meshId != -1);
	createBuffers(cmdList, alloc);
	descriptor.txtOffset = txtOffset;
	descriptor.vertexAddress = nvvk::getBufferDeviceAddress(AstraDevice.getVkDevice(), vertexBuffer.buffer);
	descriptor.indexAddress = nvvk::getBufferDeviceAddress(AstraDevice.getVkDevice(), indexBuffer.buffer);
	descriptor.materialAddress = nvvk::getBufferDeviceAddress(AstraDevice.getVkDevice(), matColorBuffer.buffer);
	descriptor.materialIndexAddress = nvvk::getBufferDeviceAddress(AstraDevice.getVkDevice(), matIndexBuffer.buffer);
}

void Astra::Mesh::createBuffers(const Astra::CommandList& cmdList, nvvk::ResourceAllocatorDma* alloc) {
	const auto& cmdBuf = cmdList.getCommandBuffer();
	VkBufferUsageFlags flag = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	VkBufferUsageFlags rayTracingFlags = flag | (AstraDevice.getRtEnabled() ? (VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) : 0);
	vertexBuffer = alloc->createBuffer(cmdBuf, vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | rayTracingFlags);
	indexBuffer = alloc->createBuffer(cmdBuf, indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | rayTracingFlags);
	matColorBuffer = alloc->createBuffer(cmdBuf, materials, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | rayTracingFlags);
	matIndexBuffer = alloc->createBuffer(cmdBuf, materialIndices, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | rayTracingFlags);
}

void Astra::Mesh::loadFromFile(const std::string& path)
{
	tinyobj::ObjReader reader;
	reader.ParseFromFile(path);
	if (!reader.Valid())
	{
		Astra::Log("Error reading obj file: " + path + ", error: " + reader.Error(), ERR);
		assert(reader.Valid());
	}

	if (!reader.Warning().empty()) {
		Astra::Log("Error reading obj file: " + path + ", error: " + reader.Warning(), WARNING);
	}

	// Collecting the material in the scene
	for (const auto& material : reader.GetMaterials())
	{
		WaveFrontMaterial m;
		m.ambient = glm::vec3(material.ambient[0], material.ambient[1], material.ambient[2]);
		m.diffuse = glm::vec3(material.diffuse[0], material.diffuse[1], material.diffuse[2]);
		m.specular = glm::vec3(material.specular[0], material.specular[1], material.specular[2]);
		m.emission = glm::vec3(material.emission[0], material.emission[1], material.emission[2]);
		m.transmittance = glm::vec3(material.transmittance[0], material.transmittance[1], material.transmittance[2]);
		m.dissolve = material.dissolve;
		m.ior = material.ior;
		m.shininess = material.shininess;
		m.illum = material.illum;
		if (!material.diffuse_texname.empty())
		{
			textures.push_back(material.diffuse_texname);
			m.textureId = static_cast<int>(textures.size()) - 1;
		}

		materials.emplace_back(m);
	}

	// If there were none, add a default
	if (materials.empty()) {
		materials.push_back(WaveFrontMaterial());
	}

	const tinyobj::attrib_t& attrib = reader.GetAttrib();

	for (const auto& shape : reader.GetShapes())
	{
		vertices.reserve(shape.mesh.indices.size() + vertices.size());
		indices.reserve(shape.mesh.indices.size() + indices.size());
		materialIndices.insert(materialIndices.end(), shape.mesh.material_ids.begin(), shape.mesh.material_ids.end());

		for (const auto& index : shape.mesh.indices)
		{
			Vertex   vertex = {};
			const float* vp = &attrib.vertices[3 * index.vertex_index];
			vertex.pos = { *(vp + 0), *(vp + 1), *(vp + 2) };

			if (!attrib.normals.empty() && index.normal_index >= 0)
			{
				const float* np = &attrib.normals[3 * index.normal_index];
				vertex.nrm = { *(np + 0), *(np + 1), *(np + 2) };
			}

			if (!attrib.texcoords.empty() && index.texcoord_index >= 0)
			{
				const float* tp = &attrib.texcoords[2 * index.texcoord_index + 0];
				vertex.texCoord = { *tp, 1.0f - *(tp + 1) };
			}

			if (!attrib.colors.empty())
			{
				const float* vc = &attrib.colors[3 * index.vertex_index];
				vertex.color = { *(vc + 0), *(vc + 1), *(vc + 2) };
			}

			vertices.push_back(vertex);
			indices.push_back(static_cast<int>(indices.size()));
		}
	}

	// Fixing material indices
	for (auto& mi : materialIndices)
	{
		if (mi < 0 || mi > materials.size())
			mi = 0;
	}


	// Compute normal when no normal were provided.
	if (attrib.normals.empty())
	{
		for (size_t i = 0; i < indices.size(); i += 3)
		{
			Vertex& v0 = vertices[indices[i + 0]];
			Vertex& v1 = vertices[indices[i + 1]];
			Vertex& v2 = vertices[indices[i + 2]];

			glm::vec3 n = glm::normalize(glm::cross((v1.pos - v0.pos), (v2.pos - v0.pos)));
			v0.nrm = n;
			v1.nrm = n;
			v2.nrm = n;
		}
	}
}
