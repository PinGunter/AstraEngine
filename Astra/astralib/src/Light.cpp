#include <Light.h>


glm::vec3& Astra::Light::getColor() {
	return _color;
}

void Astra::Light::setColor(const glm::vec3& c) {
	_color = c;
}

float& Astra::Light::getIntensity() {
	return _intensity;
}

void Astra::Light::setIntensity(float i) {
	_intensity = i;	
}

Astra::PointLight::PointLight(const glm::vec3& color, float intensity)
{
	_color = color;
	_intensity = intensity;
}

void Astra::PointLight::update()
{
	// pass
}

Astra::DirectionalLight::DirectionalLight(const glm::vec3& color, float intensity, const glm::vec3& direction)
{
	_color = color;
	_intensity = intensity;
	_direction = direction;
}

void Astra::DirectionalLight::update()
{
	// pass
}
