#include <Light.h>
#include <glm/ext/matrix_transform.hpp>

Astra::Light::Light() : Node3D()
{
	_name = std::string("Light - ") + std::to_string(_id);
}

glm::vec3 &Astra::Light::getColorRef()
{
	return _color;
}

glm::vec3 Astra::Light::getColor() const
{
	return _color;
}

void Astra::Light::setColor(const glm::vec3 &c)
{
	_color = c;
}

float &Astra::Light::getIntensityRef()
{
	return _intensity;
}

float Astra::Light::getIntensity() const
{
	return _intensity;
}

void Astra::Light::setIntensity(float i)
{
	_intensity = i;
}

Astra::LightType Astra::Light::getType() const
{
	return _type;
}

bool Astra::Light::update()
{
	return false;
}

LightSource Astra::Light::getLightSource() const
{
	LightSource src{};
	src.color = _color;
	// src.color = glm::vec4(_color, 1.0f);
	src.intensity = _intensity;
	src.position = getPosition();
	// src.position = glm::vec4(getPosition(), 1.0f);
	src.type = _type;

	return src;
}

Astra::PointLight::PointLight(const glm::vec3 &color, float intensity)
{
	_type = POINT;
	_color = color;
	_intensity = intensity;
	_name = std::string("Point Light - ") + std::to_string(_id);
}

Astra::DirectionalLight::DirectionalLight(const glm::vec3 &color, float intensity, const glm::vec3 &direction)
{
	_type = DIRECTIONAL;
	_color = color;
	_intensity = intensity;
	_direction = direction;
	_name = std::string("Directional Light - ") + std::to_string(_id);
}

glm::vec3 &Astra::DirectionalLight::getDirectionRef()
{
	return _direction;
}

glm::vec3 Astra::DirectionalLight::getDirection() const
{
	return _direction;
}

void Astra::DirectionalLight::setDirection(const glm::vec3 &dir)
{
	_direction = dir;
}

void Astra::DirectionalLight::rotate(const glm::vec3 &axis, const float &angle)
{
	auto homoDir = glm::rotate(glm::mat4(1.0f), angle, axis) * glm::vec4(_direction, 1.0f);
	_direction = {homoDir.x / homoDir.w, homoDir.y / homoDir.w, homoDir.z / homoDir.w};
}
