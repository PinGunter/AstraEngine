#pragma once
#include <Mesh.h>
#include <Light.h>
#include <Camera.h>
#include <vulkan/vulkan.h>
#include <nvvk/resourceallocator_vk.hpp>
#include <nvvk/raytraceKHR_vk.hpp>
#include <RenderContext.h>

namespace Astra
{
	/**
	 * \~spanish @brief Clase Escena. Contiene las mallas, instancias, luces y cámara. Esta es para rasterización.
	 * \~english @brief Scene class. Contains the meshes, instances, lights and camera. This is raster-only.
	 */
	class Scene
	{
	protected:
		// Models in scene
		std::vector<Mesh> _objModels;		  // the actual models (vertices, indices, etc)
		std::vector<MeshInstance> _instances; // instances of the models

		std::vector<nvvk::Texture> _textures;
		nvvk::Buffer _objDescBuffer; // Device buffer of the OBJ descriptions

		nvvk::Buffer _cameraUBO; // UBO for camera
		nvvk::Buffer _lightsUBO;
		nvvk::ResourceAllocatorDma* _alloc;

		LightsUniform _lightsUniform;
		std::vector<Light*> _lights; // multiple lights in the future
		CameraController* _camera;
		// lazy loading
		std::vector<std::pair<std::string, glm::mat4>> _lazymodels;

		virtual void createObjDescBuffer();
		virtual void createCameraUBO();
		virtual void updateCameraUBO(const CommandList& cmdList);
		virtual void createLightsUBO();
		virtual void updateLightsUBO(const CommandList& cmdList);

	public:
		Scene() = default;

		virtual void loadModel(const std::string& filepath, const glm::mat4& transform = glm::mat4(1.0f));
		virtual void init(nvvk::ResourceAllocator* alloc);
		virtual void destroy();
		virtual void addShape(Astra::Mesh& m);
		virtual void addModel(Mesh& model);
		virtual void addInstance(const MeshInstance& instance);
		virtual void removeInstance(const MeshInstance& n);
		virtual void addLight(Light* l);
		virtual void removeLight(Light* l);
		virtual void setCamera(CameraController* c);
		virtual void update(const CommandList& cmdList);
		virtual void draw(RenderContext<PushConstantRaster>& renderContext);

		const std::vector<Light*>& getLights() const;
		CameraController* getCamera() const;

		std::vector<MeshInstance>& getInstances();
		std::vector<Mesh>& getModels();
		std::vector<nvvk::Texture>& getTextures();
		nvvk::Buffer& getObjDescBuff();
		nvvk::Buffer& getCameraUBO();
		nvvk::Buffer& getLightsUBO();

		/**
		 * \~spanish @brief Resetea la escena. Se debe llamar cada vez que la App cambia de escena o si se agrega un nuevo modelo durante la ejecución.
		 * @warning No se puede llamar a esta funcion mientras se está renderizando! Asegurate de llamarla antes de beginFrame() o después de endFrame()!
		 * \~english @brief Resets the scene. Should be called every time the App switches scene or whenever a new model is loaded in runtime.
		 * @warning Don't call this method while rendering! Make sure this is called before beginFrame or after endFrame()!
		 */
		virtual void reset();

		virtual void updatePushConstantRaster(PushConstantRaster& pc);
		virtual void updatePushConstant(PushConstantRay& pc);

		virtual bool isRt() const
		{
			return false;
		};
	};

	/**
	 * \~spanish @brief Clase que agrega los elementos necesarios para el raytracing a una escena.
	 * \~english @brief Adds the necessary data and methods for raytracing a scene.
	 */
	class SceneRT : public Astra::Scene
	{
	protected:
		nvvk::RaytracingBuilderKHR _rtBuilder;
		std::vector<VkAccelerationStructureInstanceKHR> _asInstances;

	public:
		void init(nvvk::ResourceAllocator* alloc) override;
		void update(const CommandList& cmdList) override;
		/**
		 * \~spanish @brief Crea la estructura de aceleración de bajo nivel
		 * \~english @brief Creates the bottom level acceleration structure
		 */
		void createBottomLevelAS();
		/**
		 * \~spanish @brief Crea la estructura de aceleración de alto nivel
		 * \~english @brief Creates the top level acceleration structure
		 */
		void createTopLevelAS();
		/**
		 * \~spanish @brief Actualiza la estructura de aceleración de alto nivel
		 * Necesario para permitir transformaciones en tiempo de ejecución.
		 * Se debe llamar cada vez que se produzca un cambio en la matriz de tranformación de una instancia
		 * \~english @brief Updates the TLAS
		 * Needed to allow transformations in runtime
		 * It has to be called everytime an instance is transformed.
		 */
		void updateTopLevelAS(int instance_id);
		/**
		 * \~spanish @brief Reconstruye la estructura de aceleración ompleta
		 * \~english @brief Completely rebuilds the acceleation structure.
		 */
		void rebuildAS();
		VkAccelerationStructureKHR getTLAS() const;
		void destroy() override;
		bool isRt() const override
		{
			return true;
		};
		virtual void draw(RenderContext<PushConstantRay>& renderContext);

		void reset() override;
	};
}