#include <Camera.h>
#include <glm/gtx/rotate_vector.hpp>
#include <iostream>

Astra::CameraController::CameraController(Camera& cam) : _camera(cam)
{
	updateCamera();
}

void Astra::CameraController::updateCamera()
{
	_camera.eye.x = _transform[3][0];
	_camera.eye.y = _transform[3][1];
	_camera.eye.z = _transform[3][2];
	_camera.viewMatrix = glm::lookAt(_camera.eye, _camera.centre, _camera.up);
}

glm::mat4 Astra::CameraController::getViewMatrix() const
{
	return _camera.viewMatrix;
}

glm::mat4 Astra::CameraController::getProjectionMatrix() const
{
	assert(_height != 0.0 && _width != 0.0);
	float aspectRatio = _width / static_cast<float>(_height);
	glm::mat4 proj = glm::perspectiveRH_ZO(glm::radians(_camera._fov), aspectRatio, _camera._near, _camera._far);
	proj[1][1] *= -1; // vulkan shenanigans ;)
	return proj;
}

void Astra::CameraController::setLookAt(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up)
{
	_camera.eye = eye;
	_camera.centre = center;
	_camera.up = up;
	_transform = glm::translate(glm::mat4(1.0f), eye);
	updateCamera();
}

void Astra::CameraController::update(/*render pipeline*/)
{
	// in the future a render pipeline will be passed so that we can update the render pipeline's camera info
	updateCamera();
}


Astra::FPSCameraController::FPSCameraController(Camera& cam) : CameraController(cam)
{
}

void Astra::FPSCameraController::mouseMovement(float x, float y)
{
	// we are only going to allow moving the camera while the user right clicks

}

void Astra::FPSCameraController::move(float qty)
{
}

Astra::OrbitCameraController::OrbitCameraController(Camera& cam) : CameraController(cam)
{
}

void Astra::OrbitCameraController::mouseMovement(float x, float y)
{
	rotate(glm::vec3(0, 1, 0), x * 0.01);
	auto newpos = _transform * glm::vec4(5.0f, 5.0f, 5.0f, 0.0f);
	_transform[3] = newpos;
	updateCamera();

}
