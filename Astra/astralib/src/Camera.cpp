#include <Camera.h>
#include <glm/gtx/rotate_vector.hpp>
#include <iostream>


Astra::CameraController::CameraController(Camera& cam) : _camera(cam)
{
	update();
}

void Astra::CameraController::update()
{
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
	update();
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
	//// if mouse moves in x direction, rotate camera in y axis
	//glm::vec3 position = _camera.centre - _camera.eye;
	//_camera.centre = glm::rotateY(position, _sens * x);
	//update();

	x *= glm::two_pi<float>();
	y *= glm::two_pi<float>();

	// Get the camera
	glm::vec3 origin(_camera.centre);
	glm::vec3 position(_camera.eye);

	// Get the length of sight
	glm::vec3 centerToEye(position - origin);
	float     radius = glm::length(centerToEye);
	centerToEye = glm::normalize(centerToEye);
	glm::vec3 axe_z = centerToEye;

	// Find the rotation around the UP axis (Y)
	glm::mat4 rot_y = glm::rotate(glm::mat4(1), -x, _camera.up);

	// Apply the (Y) rotation to the eye-center vector
	centerToEye = rot_y * glm::vec4(centerToEye, 0);

	// Find the rotation around the X vector: cross between eye-center and up (X)
	glm::vec3 axe_x = glm::normalize(glm::cross(_camera.up, axe_z));
	glm::mat4 rot_x = glm::rotate(glm::mat4(1), -y, axe_x);

	// Apply the (X) rotation to the eye-center vector
	glm::vec3 vect_rot = rot_x * glm::vec4(centerToEye, 0);

	if (glm::sign(vect_rot.x) == glm::sign(centerToEye.x))
		centerToEye = vect_rot;

	// Make the vector as long as it was originally
	centerToEye *= radius;

	// Finding the new position
	glm::vec3 newPosition = centerToEye + origin;


	_camera.eye = newPosition;  // Normal: change the position of the camera
	update();

}

