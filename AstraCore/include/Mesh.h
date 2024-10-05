#pragma once
#include <Node3D.h>
#include <string>
#include "nvvk/resourceallocator_vk.hpp"
#include <host_device.h>
#include <vector>
#include <vulkan/vulkan.h>
#include <CommandList.h>
namespace Astra
{
	/**
	* @struct Geometry
	* \~spanish @brief Representa una geometría sencilla, vector de vertices y caras
	* \~english @brief Represents a simple geometry, just a vertex and face vectors
	*/
	struct Geometry {
		std::vector<glm::ivec3> indices;
		std::vector<glm::vec3> vertices;
		std::vector<glm::vec3> normals;
	};


	/**
	 * @struct Mesh
	 * \~spanish @brief Representa un modelo 3D como una malla de vértices y triángulos
	 * Contiene la información necesaria para acceder tanto desde la CPU como GPU
	 * \~english @brief Represents a 3D model using a triangle mesh representation
	 * Contains all the necessary info for it to be accessed from both host and device.
	 */
	struct Mesh
	{
		/**
		 * \~spanish @brief El identificador de la malla en la escena.
		 * @warning Debe ser único por escena y estar siempre inicializado!
		 * \~english @brief Scene mesh id.
		 * @warning This is must be unique in a scene basis and has to always be initialized.
		 */
		int meshId{-1};

		// CPU side
		/**
		 * \~spanish @brief Vector de índices en CPU
		 * \~english @brief Index vector on CPU
		 */
		std::vector<uint32_t> indices;
		/**
		 * \~spanish @brief Vector de vértices en CPU
		 * \~english @brief Vertex vector on CPU
		 */
		std::vector<Vertex> vertices;
		/**
		 * \~spanish @brief Vector de materiales en CPU
		 * \~english @brief Materials vector on CPU
		 */
		std::vector<WaveFrontMaterial> materials;
		/**
		 * \~spanish @brief Vector de índices de materiales en CPU
		 * \~english @brief Material index vector on CPU
		 */
		std::vector<int32_t> materialIndices;
		/**
		 * \~spanish @brief Vector de texturas en CPU
		 * \~english @brief Texture vector on CPU
		 */
		std::vector<nvvk::Texture> textures;
		/**
		 * \~spanish @brief Vector de texturas (paths) en CPU
		 * \~english @brief Texture path vector on CPU
		 */
		std::vector<std::string> texturePaths;

		// CPU - GPU side
		/**
		 * \~spanish @brief Buffer de vértices en GPU
		 * \~english @brief Vertex buffer on Device
		 */
		nvvk::Buffer vertexBuffer;
		/**
		 * \~spanish @brief Buffer de índices en GPU
		 * \~english @brief Index buffer on Device
		 */
		nvvk::Buffer indexBuffer;
		/**
		 * \~spanish @brief Buffer de materiales en GPU
		 * \~english @brief Materials buffer on Device
		 */
		nvvk::Buffer matColorBuffer;
		/**
		 * \~spanish @brief Buffer de índice de materiales en GPU
		 * \~english @brief Material index buffer on Device
		 */
		nvvk::Buffer matIndexBuffer;

		// GPU side
		/**
		 * \~spanish @brief Contiene las direcciones de memoria de los buffers en la GPU
		 * \~english @brief Contains the GPU memory addresses for the buffers.
		 */
		ObjDesc descriptor{}; // gpu buffer addresses

		/**
		 * \~spanish @brief Dibuja el modelo
		 * \~english @brief Draws the model
		 */
		void draw(const CommandList &cmdList) const;
		/**
		 * \~spanish @brief Crea los buffers y almacena las direcciones de memoria de estos
		 * \~english @brief Creates the buffers and stores the device buffer addresses
		 */
		void create(const Astra::CommandList &cmdList, nvvk::ResourceAllocatorDma *alloc, uint32_t txtOffset);
		
		/**
		 * \~spanish @brief Carga un modelo obj y almacena la información en los vectores de la CPU
		 * \~english @brief Loads an obj file and stores the data in the CPU vectors
		 */
		void loadFromFile(const std::string &path);

		/**
		* \~spanish @brief Inicializa un mesh a partir de una geometria y un material, para figuras simples
		* \~english @brief Initializes a mesh from a geometry and material, for simple objects
		*/
		void fromGeoMat(const Astra::Geometry& geom, const WaveFrontMaterial &material);

	private:
		/**
		 * \~spanish @brief Crea los buffers
		 * \~english @brief Creates the buffers
		 */
		void createBuffers(const Astra::CommandList& cmdList, nvvk::ResourceAllocatorDma* alloc);
	};

	/**
	 * \~spanish @brief Clase que representa una instancia de una malla en una escena
	 * \~english @brief Represents an instance of a mesh in a scene.
	 */
	class MeshInstance : public Node3D
	{
	protected:
		bool _visible{true};
		/**
		 * \~spanish @brief Id de la malla que representa
		 * \~english @brief Id of the mesh that is instancing
		 */
		uint32_t _mesh; // index reference to mesh in a scene.
	public:
		MeshInstance(uint32_t mesh, const glm::mat4 &transform = glm::mat4(1.0f), const std::string &name = "");

		MeshInstance &operator=(const MeshInstance &other);

		void setVisible(bool v);

		bool getVisible() const;
		bool &getVisibleRef();
		uint32_t getMeshIndex() const;

		bool update(float delta) override;
		void destroy() override;
		void updatePushConstantRaster(PushConstantRaster &pc) const override;
		void updatePushConstantRT(PushConstantRay &pc) const override;
	};
}