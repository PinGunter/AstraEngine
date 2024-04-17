#include <Camera.h>
#include <glm/gtx/rotate_vector.hpp>

Astra::FPSCameraController::FPSCameraController(Camera& cam) : CameraController(cam)
{
}

void Astra::FPSCameraController::mouseMovement(float x, float y, MouseClick mouseClick)
{
	// we are only going to allow moving the camera while the user right clicks

	if (mouseClick == RIGHT) {
		// X Axis mouse movement means rotating around Y axis
		rotate(glm::vec3(0, 1, 0), _sens * x);
		_camera.lookAt = glm::rotateY(_camera.lookAt, _sens * x);

		// Y Axis mouse movement means rotating around X axis
		rotate(glm::vec3(1, 0, 0), _sens * y);
		_camera.lookAt = glm::rotateX(_camera.lookAt, _sens * y);
	}

}

void Astra::FPSCameraController::move(float qty)
{
}

Astra::OrbitCameraController::OrbitCameraController(Camera& cam) : CameraController(cam)
{
}

void Astra::OrbitCameraController::mouseMovement(float x, float y, MouseClick mouseClick)
{
}

glm::mat4 Astra::CameraController::getViewMatrix() const
{
	return glm::mat4();
}

glm::mat4 Astra::CameraController::getProjectionMatrix() const
{
	return glm::mat4();
}
