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
		float _near{ 0.1f };
		float _far{ 1000.0f };
		float _fov{ 60.0f };
		glm::vec3 eye{ 2.0f };
		glm::vec3 up{ 0.0f,1.0f,0.0f };
		glm::vec3 centre{ 0.0f };

		glm::mat4 viewMatrix{ 1.0f };
	};

	class CameraController {
	protected:
		Camera& _camera;
		float _sens = 0.001f;
		uint32_t _width, _height;
	public:
		CameraController(Camera& cam);
		virtual void mouseMovement(float x, float y) = 0;
		void update(/* renderpipeline */);
		void setSens(float s) { _sens = s; }
		float getSens() const { return _sens; }
		void setWindowSize(uint32_t w, uint32_t h) { _width = w, _height = h; }
		glm::mat4 getViewMatrix() const;
		glm::mat4 getProjectionMatrix() const;
		void setLookAt(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up);
	};

	class FPSCameraController : public CameraController {
	public:
		FPSCameraController(Camera& cam);
		void mouseMovement(float x, float y) override;
		void move(float qty);
	};

	class OrbitCameraController : public CameraController {
	public:
		OrbitCameraController(Camera& cam);
		void mouseMovement(float x, float y) override;
	};
}