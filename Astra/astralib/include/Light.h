#pragma once
#include <Node3D.h>
#include <host_device.h>

namespace Astra {
	enum LightType {POINT = 0, DIRECTIONAL = 1};
	class Light : public Node3D {
	protected:
		glm::vec3 _color;
		float _intensity;
		LightType _type;
	public:
		glm::vec3& getColor();
		void setColor(const glm::vec3& c);
		float&  getIntensity();
		void setIntensity(float i);
		LightType getType() const;

		void updatePushConstantRaster(PushConstantRaster& pc) const override;
		void updatePushConstantRT(PushConstantRay& pc) const override;
		void update() override;
	};

	class PointLight : public Light {
	public:
		PointLight(const glm::vec3& color, float intensity);
	};

	class DirectionalLight : public Light {
	protected:
		glm::vec3 _direction;
	public:
		DirectionalLight(const glm::vec3& color, float intensity, const glm::vec3& direction);
	};
}