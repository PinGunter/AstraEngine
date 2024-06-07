#pragma once
#include <Mesh.h>
#include <Light.h>
#include <Camera.h>
#include <vulkan/vulkan.h>
#include <nvvk/resourceallocator_vk.hpp>
#include <App.h>

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
		VkAccelerationStructureKHR _tlas;
		glm::mat4 _transform;
	public:
		Scene();
		void destroy();
		void addModel(const HostModel& model);
		void addInstance(const MeshInstance & instance);
		void addObjDesc(const ObjDesc& objdesc);
		void removeNode(const MeshInstance& n);
		void addLight(Light* l);
		void setCamera(CameraController* c);
		void setTLAS(VkAccelerationStructureKHR tlas);

		Light* getLight() const;
		CameraController* getCamera() const;
		VkAccelerationStructureKHR getTLAS() const;

		const std::vector<MeshInstance>& getInstances() const;
		const std::vector<HostModel>& getModels() const;
		const std::vector<ObjDesc>& getObjDesc() const;
		std::vector<nvvk::Texture>& getTextures();
		nvvk::Buffer& getObjDescBuff();

		glm::mat4& getTransformRef();
		const glm::mat4& getTransformRef() const;


		void updatePushConstantRaster(PushConstantRaster& pc);
		void updatePushConstant(PushConstantRay& pc);
	};
}