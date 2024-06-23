#include <Light.h>
#include <glm/ext/matrix_transform.hpp>

Astra::Light::Light() :Node3D() {
	_name = std::string("Light - ") + std::to_string(_id);
}

glm::vec3& Astra::Light::getColorRef() {
	return _color;
}

glm::vec3 Astra::Light::getColor() const
{
	return _color;
}

void Astra::Light::setColor(const glm::vec3& c) {
	_color = c;
}

float& Astra::Light::getIntensityRef() {
	return _intensity;
}

float Astra::Light::getIntensity() const
{
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
	pc.r = _color.x;
	pc.g = _color.y;
	pc.b = _color.z;
	pc.lightIntensity = _intensity;
	pc.lightPosition = getPosition();
	pc.lightType = static_cast<int>(_type);
}

void Astra::Light::updatePushConstantRT(PushConstantRay& pc) const
{
	//pc.lightColor = _color;
	pc.r = _color.x;
	pc.g = _color.y;
	pc.b = _color.z;
	pc.lightIntensity = _intensity;
	pc.lightPosition = getPosition();
	pc.lightType = static_cast<int>(_type);
}

bool Astra::Light::update()
{
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

glm::vec3& Astra::DirectionalLight::getDirectionRef()
{
	return _direction;
}

glm::vec3 Astra::DirectionalLight::getDirection() const
{
	return _direction;
}

void Astra::DirectionalLight::setDirection(const glm::vec3& dir)
{
	_direction = dir;
}

void Astra::DirectionalLight::rotate(const glm::vec3& axis, const float& angle)
{
	auto homoDir = glm::rotate(glm::mat4(1.0f), angle, axis) * glm::vec4(_direction, 1.0f);
	_direction = { homoDir.x / homoDir.w, homoDir.y / homoDir.w, homoDir.z / homoDir.w };
}

void Astra::DirectionalLight::updatePushConstantRaster(PushConstantRaster& pc) const
{
	Light::updatePushConstantRaster(pc);
	pc.lightPosition = _direction;
}

void Astra::DirectionalLight::updatePushConstantRT(PushConstantRay& pc) const
{
	Light::updatePushConstantRT(pc);
	pc.lightPosition = _direction;
}

bool Astra::DirectionalLight::update()
{
	rotate(glm::vec3(1, 0, 0), 0.0004f);
	_transform = glm::translate(glm::mat4(1.0f), _direction);
	return true;
}

