#pragma once
#include <Mesh.h>
#include <Light.h>
#include <Camera.h>
#include <vulkan/vulkan.h>
#include <nvvk/resourceallocator_vk.hpp>
#include <nvvk/raytraceKHR_vk.hpp>

namespace Astra {
	class Scene {
	protected:
		// Models in scene
		std::vector<Mesh> _objModels;  // the actual models (vertices, indices, etc)
		std::vector<MeshInstance> _instances; // instances of the models

		std::vector<nvvk::Texture> _textures;
		nvvk::Buffer _objDescBuffer;  // Device buffer of the OBJ descriptions

		nvvk::ResourceAllocatorDma* _alloc;

		std::vector< Light*> _lights; // multiple lights in the future
		CameraController* _camera;
		// lazy loading
		std::vector<std::pair<std::string, glm::mat4>> _lazymodels;

		virtual void createObjDescBuffer();
	public:
		Scene() = default;

		virtual void loadModel(const std::string& filepath, const glm::mat4& transform = glm::mat4(1.0f));
		virtual void init(nvvk::ResourceAllocator* alloc);
		virtual void destroy();
		virtual void addModel(const Mesh& model);
		virtual void addInstance(const MeshInstance& instance);
		virtual void removeNode(const MeshInstance& n);
		virtual void addLight(Light* l);
		virtual void setCamera(CameraController* c);
		virtual void update();

		const std::vector<Light*>& getLights() const;
		CameraController* getCamera() const;

		std::vector<MeshInstance>& getInstances();
		std::vector<Mesh>& getModels();
		std::vector<nvvk::Texture>& getTextures();
		nvvk::Buffer& getObjDescBuff();


		virtual void updatePushConstantRaster(PushConstantRaster& pc);
		virtual void updatePushConstant(PushConstantRay& pc);

		virtual bool isRt() const {
			return false;
		};
	};

	class DefaultSceneRT : public Astra::Scene {
	protected:
		nvvk::RaytracingBuilderKHR _rtBuilder;
		std::vector<VkAccelerationStructureInstanceKHR> _asInstances;


	public:
		void init(nvvk::ResourceAllocator* alloc) override;
		void update() override;
		void createBottomLevelAS();
		void createTopLevelAS();
		void updateTopLevelAS(int instance_id);
		VkAccelerationStructureKHR getTLAS() const;
		void destroy() override;
		bool isRt() const override {
			return true;
		};
	};
}