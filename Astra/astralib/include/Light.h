#pragma once
#include <Node3D.h>

namespace Astra {
	class Light : public Node3D {
	protected:
		glm::vec3 _color;
		float _intensity;
	public:
		Light();
		glm::vec3& getColor();
		void setColor(const glm::vec3& c);
		float&  getIntensity();
		void setIntensity(float i);
	};

	class PointLight : public Light {
	public:
		PointLight(const glm::vec3& color, float intensity);
		void update(/*renderpipeline*/) override;
	};

	class DirectionalLight : public Light {
	protected:
		glm::vec3 _direction;
	public:
		DirectionalLight(const glm::vec3& color, float intensity, const glm::vec3& direction);
		void update(/*renderpipeline*/) override;
	};
}