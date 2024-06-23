#pragma once
#include <Node3D.h>
#include <string>
#include "nvvk/resourceallocator_vk.hpp"
#include <host_device.h>
#include <vector>
#include <vulkan/vulkan.h>
#include <CommandList.h>
namespace Astra {

	struct Mesh {
		int meshId{ -1 };

		// CPU side
		std::vector<uint32_t> indices;
		std::vector<Vertex> vertices;
		std::vector<WaveFrontMaterial> materials;
		std::vector<int32_t> materialIndices;
		std::vector<std::string> textures;

		// CPU - GPU side
		nvvk::Buffer vertexBuffer;    // Device buffer of all 'Vertex'
		nvvk::Buffer indexBuffer;     // Device buffer of the indices forming triangles
		nvvk::Buffer matColorBuffer;  // Device buffer of array of 'Wavefront material'
		nvvk::Buffer matIndexBuffer;  // Device buffer of array of 'Wavefront material'

		// GPU side
		ObjDesc descriptor{}; // gpu buffer addresses

		void draw(const CommandList& cmdList) const;
		void create(const Astra::CommandList& cmdList, nvvk::ResourceAllocatorDma* alloc, uint32_t txtOffset);
		void createBuffers(const Astra::CommandList& cmdList, nvvk::ResourceAllocatorDma* alloc); // fills the gpu buffers with the vector data
		void loadFromFile(const std::string& path);
	};

	/**
	* Class that represents an instance of a mesh
	*/
	class MeshInstance : public Node3D {
	protected:
		bool _visible{ true };
		uint32_t _mesh; // index reference to mesh in the app, to a **VULKAN MESH**
	public:
		/**
		* Constructor, takes mesh reference, name and transform for the instance
		* @brief full constructor
		* @param mesh the mesh reference
		* @param name the name of the instance object
		* @param transform the transform matrix
		*/
		MeshInstance(uint32_t mesh, const glm::mat4& transform = glm::mat4(1.0f), const std::string& name = "");

		MeshInstance& operator=(const MeshInstance& other);

		void setVisible(bool v);

		bool getVisible() const;
		bool& getVisibleRef();
		uint32_t getMeshIndex() const;

		bool update() override;
		void destroy() override;
		void updatePushConstantRaster(PushConstantRaster& pc) const override;
		void updatePushConstantRT(PushConstantRay& pc) const override;
	};
}