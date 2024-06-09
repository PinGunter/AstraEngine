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

Astra::LightType Astra::Light::getType() const
{
	return _type;
}

void Astra::Light::updatePushConstantRaster(PushConstantRaster& pc) const
{
	pc.lightColor = _color;
	pc.lightIntensity = _intensity;
	pc.lightPosition = getPosition();
	pc.lightType = static_cast<int>(_type);
}

void Astra::Light::updatePushConstantRT(PushConstantRay& pc) const
{
	pc.lightColor = _color;
	pc.lightIntensity = _intensity;
	pc.lightPosition = getPosition();
	pc.lightType = static_cast<int>(_type);
}

void Astra::Light::update()
{
	// pass
}

Astra::PointLight::PointLight(const glm::vec3& color, float intensity)
{
	_type = POINT;
	_color = color;
	_intensity = intensity;
}

Astra::DirectionalLight::DirectionalLight(const glm::vec3& color, float intensity, const glm::vec3& direction)
{
	_type = DIRECTIONAL;
	_color = color;
	_intensity = intensity;
	_direction = direction;
}
