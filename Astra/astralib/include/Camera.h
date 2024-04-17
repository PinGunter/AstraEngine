#pragma once
#include <Node3D.h>

namespace Astra {
	enum MouseClick {
		NOCLICK,
		LEFT,
		MIDDLE,
		RIGHT
	};

	struct Camera {
		float aspectRatio;
		float near, far;
		float fov;
		glm::vec3 lookAt;
		glm::vec3 up;
	};

	class CameraController : public Node3D {
	protected:
		Camera& _camera;
		float _sens = 0.01f;
	public:
		CameraController(Camera& cam) : _camera(cam) {}
		virtual void mouseMovement(float x, float y, MouseClick mouseClick) = 0;
		void setSens(float s) { _sens = s; }
		float getSens() const { return _sens; }
		void setAspectRatio(float ar) { _camera.aspectRatio = ar; }
		glm::mat4 getViewMatrix() const;
		glm::mat4 getProjectionMatrix() const;
	};

	class FPSCameraController : public CameraController {
	public:
		FPSCameraController(Camera& cam);
		void mouseMovement(float x, float y, MouseClick mouseClick) override;
		void move(float qty);
	};

	class OrbitCameraController : public CameraController {
		OrbitCameraController(Camera& cam);
		void mouseMovement(float x, float y, MouseClick mouseClick) override;
	};
}