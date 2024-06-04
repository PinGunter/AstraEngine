#pragma once
#include <Node3D.h>
#include <glm/gtc/constants.hpp>

namespace Astra {
	enum MouseClick {
		NOCLICK,
		LEFT,
		MIDDLE,
		RIGHT
	};

	struct Camera {
		float _near{ 0.1f };
		float _far{ 1000.0f };
		float _fov{ 60.0f };
		glm::vec3 _eye{ 2.0f };
		glm::vec3 _up{ 0.0f,1.0f,0.0f };
		glm::vec3 _centre{ 0.0f };

		glm::mat4 viewMatrix{ 1.0f };
	};

	class CameraController : public Node3D {
	protected:
		Camera& _camera;
		float _sens = 0.01f;
		uint32_t _width, _height;
		void updateCamera(bool from_transform = true);
	public:
		CameraController(Camera& cam);
		void setSens(float s) { _sens = s; }
		float getSens() const { return _sens; }
		void setWindowSize(uint32_t w, uint32_t h) { _width = w, _height = h; }
		glm::mat4 getViewMatrix() const;
		glm::mat4 getProjectionMatrix() const;
		void setLookAt(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up);

		float getNear() const;
		float fetFar() const;
		float getFov() const;
		glm::vec3 getEye() const;
		glm::vec3 getUp() const;
		glm::vec3 getCentre() const;

		float& getNear();
		float& fetFar();
		float& getFov();
		glm::vec3& getEye();
		glm::vec3& getUp();
		glm::vec3& getCentre();

		void setNear(float n);
		void setFar(float f);
		void setFov(float f);


		void update(/* renderpipeline */) override;
	};

	class FPSCameraController : public CameraController {
	public:
		FPSCameraController(Camera& cam);
		void mouseMovement(float x, float y);
		void move(float qty);
	};

	class OrbitCameraController : public CameraController {
	private:
		float maxXRotation = glm::pi<float>() * 0.4f;
		float minXRotation = -glm::pi<float>() * 0.4f;
	public:
		OrbitCameraController(Camera& cam);
		void orbit(float x, float y);
		void zoom(float increment);
		void pan(float x, float y);
	};
}