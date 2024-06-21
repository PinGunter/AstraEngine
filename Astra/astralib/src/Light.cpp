#include <Light.h>

Astra::Light::Light() :Node3D() {
	_name = std::string("Light - ") + std::to_string(_id);
}

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

bool Astra::Light::update()
{
	// pass
	return false;
}

Astra::PointLight::PointLight(const glm::vec3& color, float intensity)
{
	_type = POINT;
	_color = color;
	_intensity = intensity;
	_name = std::string("Point Light - ") + std::to_string(_id);
}

Astra::DirectionalLight::DirectionalLight(const glm::vec3& color, float intensity, const glm::vec3& direction)
{
	_type = DIRECTIONAL;
	_color = color;
	_intensity = intensity;
	_direction = direction;
	_name = std::string("Directional Light - ") + std::to_string(_id);
}

