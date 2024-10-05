#pragma once
#include <Node3D.h>

namespace Astra
{
	enum LightType
	{
		POINT = 0,
		DIRECTIONAL = 1
	};

	/**
	 * @class Light
	 * \~spanish @brief Clase abstracta que representa una luz en una escena. Hereda de Node3D
	 * \~english @brief Asbtract class that represents a light in a scene. Inherits from Node3D
	 */
	class Light : public Node3D
	{
	protected:
		glm::vec3 _color;
		float _intensity;
		LightType _type;

	public:
		Light();
		glm::vec3& getColorRef();
		glm::vec3 getColor() const;
		void setColor(const glm::vec3& c);
		float& getIntensityRef();
		float getIntensity() const;
		void setIntensity(float i);
		LightType getType() const;

		bool update(float delta) override;

		/**
		 * \~spanish @brief Devuelve un objeto de tipo LightSource, que está compartido entre CPU y GPU.
		 * \~english @brief Returns a LightSource object. This object is declared in a Host-Device shared include file.
		 */
		virtual LightSource getLightSource() const;
	};

	/**
	 * \~spanish @brief Implementa la clase Light y la especializa en una luz puntual
	 * \~english @brief Implements the Light class. It represents a point light.
	 */
	class PointLight : public Light
	{
	public:
		PointLight(const glm::vec3& color, float intensity);
	};

	/**
	 * \~spanish @brief Implementa la clase Light y la especializa en una luz direccional
	 * \~english @brief Implements the Light class. It represents a directional light.
	 */
	class DirectionalLight : public Light
	{
	protected:
		glm::vec3 _direction;

	public:
		DirectionalLight(const glm::vec3& color, float intensity, const glm::vec3& direction);
		glm::vec3& getDirectionRef();
		glm::vec3 getDirection() const;
		void setDirection(const glm::vec3& dir);
		LightSource getLightSource() const override;

		void rotate(const glm::vec3& axis, const float& angle) override;
	};
}