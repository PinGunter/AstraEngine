#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <vulkan/vulkan.h>
#include <host_device.h>

namespace Astra
{

	/**
	 * \~spanish @brief Clase abstracta que representa cualquier objeto 3D en una escena. Todos los métodos son simples y hacen lo que dice el nombre de este.
	 * \~english @brief Abstract class that represents any 3D object in the scene. These are very simple methods and they do as their name says.
	 */
	class Node3D
	{
	protected:
		static uint32_t NodeCount;
		glm::mat4 _transform;
		std::vector<Node3D*> _children;
		std::string _name;
		uint32_t _id;

	public:
		Node3D(const glm::mat4& transform = glm::mat4(1.0f), const std::string& name = "");
		virtual void destroy() {}

		bool operator==(const Node3D& other);

		virtual void addChild(Node3D* child);
		virtual void removeChild(const Node3D& child);

		// TRANSFORM OPERATIONS

		virtual void rotate(const glm::vec3& axis, const float& angle);

		virtual void scale(const glm::vec3& scaling);

		virtual void translate(const glm::vec3& position);

		// GETTERS
		virtual glm::vec3 getPosition() const;
		virtual glm::vec3 getRotation() const;
		virtual glm::vec3 getScale() const;
		virtual glm::vec3 getPosition();
		virtual glm::vec3 getRotation();
		virtual glm::vec3 getScale();

		glm::mat4& getTransformRef() { return _transform; }

		const glm::mat4& getTransform() const { return _transform; }

		std::vector<Node3D*>& getChildren() { return _children; }

		std::string& getNameRef();
		std::string getName() const;

		uint32_t getID() const;

		// SETTERS
		void setName(const std::string& n) { _name = n; }

		/**
		 * \~spanish @brief Método que se llama en cada iteración del bucle de la aplicación
		 * @return Debe devolver true si se ha modificado algo del objeto o no. Útil para reconstruir estructuras de aceleración.
		 * \~english @brief This method is callen in every iteration of the app run loop
		 * @return Should return true if it modifies anything within the Node so that it's known. Useful for rebuilding acceleration structures.
		 */
		virtual bool update(float delta) { return false; };

		/**
		 * \~spanish @brief Método para actualizar la push constant de rasterización
		 * \~english @brief Method to update the push constant for raster
		 */
		virtual void updatePushConstantRaster(PushConstantRaster& pc) const {};
		/**
		 * \~spanish @brief Método para actualizar la push constant de ray-tracing
		 * \~english @brief Method to update the push constant for ray-tracing
		 */
		virtual void updatePushConstantRT(PushConstantRay& pc) const {};
	};
}