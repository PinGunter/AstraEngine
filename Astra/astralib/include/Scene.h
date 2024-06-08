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
		std::vector<HostModel> _objModels;  // the actual models
		std::vector<MeshInstance> _instances; // instances of the models
		std::vector<ObjDesc> _objDesc; // device access

		std::vector<nvvk::Texture> _textures;
		nvvk::Buffer _objDescBuffer;  // Device buffer of the OBJ descriptions


		Light* _light; // multiple lights in the future
		CameraController* _camera;
		glm::mat4 _transform;
	public:
		Scene();

		Scene& operator=(const Scene& s);

		virtual void destroy();
		virtual void addModel(const HostModel& model);
		virtual void addInstance(const MeshInstance & instance);
		virtual void addObjDesc(const ObjDesc& objdesc);
		virtual void removeNode(const MeshInstance& n);
		virtual void addLight(Light* l);
		virtual void setCamera(CameraController* c);

		Light* getLight() const;
		CameraController* getCamera() const;

		const std::vector<MeshInstance>& getInstances() const;
		const std::vector<HostModel>& getModels() const;
		const std::vector<ObjDesc>& getObjDesc() const;
		std::vector<nvvk::Texture>& getTextures();
		nvvk::Buffer& getObjDescBuff();

		glm::mat4& getTransformRef();
		const glm::mat4& getTransformRef() const;


		virtual void updatePushConstantRaster(PushConstantRaster& pc);
		virtual void updatePushConstant(PushConstantRay& pc);

		virtual bool isRt() const {
			return false;
		};
	};

	class SceneRT : public Scene {
	protected:
		nvvk::RaytracingBuilderKHR _rtBuilder;
		std::vector<VkAccelerationStructureInstanceKHR> _asInstances;

	public:
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