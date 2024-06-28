#pragma once
#include <Node3D.h>
#include <glm/gtc/constants.hpp>
#include <host_device.h>

namespace Astra
{
	/**
	 * @struct Camera
	 * \~spanish @brief Contiene la información básica de la cámara
	 * \~english @brief Contains the camera parameters

	 */
	struct Camera
	{
		float nearPlane{0.1f};
		float farPlane{1000.0f};
		float fov{60.0f};
		glm::vec3 eye{2.0f};
		glm::vec3 up{0.0f, 1.0f, 0.0f};
		glm::vec3 centre{0.0f};

		glm::mat4 viewMatrix{1.0f};
	};

	/**
	 * @class CameraController
	 * \~spanish @brief Manipula la cámara según el comportamiento definido en el método update de las clases que lo implementen.
	 * \~english @brief Controls the camera based on the behaviour defined in its derived classes' update method.
	 */
	class CameraController
	{
	protected:
		Camera &_camera;
		float _sens = 0.01f;
		uint32_t _width, _height;
		void updateCamera();

	public:
		CameraController(Camera &cam);
		void setSens(float s) { _sens = s; }
		float getSens() const { return _sens; }
		void setWindowSize(uint32_t w, uint32_t h) { _width = w, _height = h; }
		const glm::mat4 &getViewMatrix() const;
		glm::mat4 getProjectionMatrix() const;
		void setLookAt(const glm::vec3 &eye, const glm::vec3 &center, const glm::vec3 &up);

		float getNear() const;
		float fetFar() const;
		float getFov() const;
		const glm::vec3 &getEye() const;
		const glm::vec3 &getUp() const;
		const glm::vec3 &getCentre() const;

		Camera getCamera() const;

		float &getNearRef();
		float &fetFarRef();
		float &getFovRef();
		glm::vec3 &getEyeRef();
		glm::vec3 &getUpRef();
		glm::vec3 &getCentreRef();

		void setNear(float n);
		void setFar(float f);
		void setFov(float f);

		virtual bool update();
		CameraUniform getCameraUniform();
	};
	/**
	 * @class FreeCameraController
	 * \~spanish @brief Proporciona controles de cámara libre. Se controla con WASD, para movimiento, QE para altura, CTRL para aumentar la velocidad y el ratón (con el click derecho) para girar
	 * \~english @brief Free Camera controls for the camera. WASD and QE for movement, CTRL for speed-up, right-click drag for rotating the camera.
	 */
	class FreeCameraController : public CameraController
	{
	protected:
		float _speed = 0.1f;
		void move(bool front, bool back, bool right, bool left, bool up, bool down, float speed);
		void rotate(float dx, float dy);

	public:
		FreeCameraController(Camera &cam);
		bool update() override;
	};
	/**
	 * @class OrbitCameraController
	 * \~spanish @brief Orbita alrededor de un punto con el click derecho. Zoom con la rueda. Se puede desplazar el punto al que mira con el botón de la rueda del ratón.
	 * \~english @brief Orbits around a point with right click drag. Wheel zooms in or out. Middle mouse changes the point around where the camera orbits.
	 */
	class OrbitCameraController : public CameraController
	{
	protected:
		float maxXRotation = glm::pi<float>() * 0.4f;
		float minXRotation = -glm::pi<float>() * 0.4f;
		void orbit(float x, float y);
		void pan(float x, float y);
		void zoom(float increment);

	public:
		OrbitCameraController(Camera &cam);
		bool update() override;
	};
}