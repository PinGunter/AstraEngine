#pragma once
#include <Node3D.h>
#include <string>
#include "nvvk/resourceallocator_vk.hpp"
#include <host_device.h>
#include <vector>
#include <nvvk/commands_vk.hpp>
namespace Astra {

	/**
		Represents a mesh in host memory, easy to manipulate and create
		Has method to change to vulkan mesh, in gpu memory
	*/
	struct Mesh {
		std::vector<uint32_t> indices;
		std::vector<Vertex> vertices;
		std::vector<WaveFrontMaterial> materials;
		std::vector<int32_t> materialIndices;
		std::vector<std::string> textures;

		// TODO should be in future device/app class
		/*VulkanMesh toVulkanMesh() {
		}*/
	};
	/**
	* Struct that stores the neccesary information for vulkan for a mesh
	* It has to be created within an App since it needs connection to the vulkan device
	*/
	struct VulkanMesh {
		uint32_t     nbIndices{ 0 };
		uint32_t     nbVertices{ 0 };
		nvvk::Buffer vertexBuffer;    // Device buffer of all 'Vertex'
		nvvk::Buffer indexBuffer;     // Device buffer of the indices forming triangles
		nvvk::Buffer matColorBuffer;  // Device buffer of array of 'Wavefront material'
		nvvk::Buffer matIndexBuffer;  // Device buffer of array of 'Wavefront material'
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
		MeshInstance(uint32_t index, const glm::mat4& transform = glm::mat4(1.0f), const std::string& name = "");


		// SETTERS
		void setVisible(bool v);

		// GETTERS
		bool getVisible() const;

		bool& getVisible();

		uint32_t getMeshIndex() const;

		void update() override;

	};
}