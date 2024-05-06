#include <Camera.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <iostream>

Astra::CameraController::CameraController(Camera& cam) : _camera(cam)
{
	updateCamera();
}

void Astra::CameraController::updateCamera(bool from_transform)
{
	// we can update the cam params either from the transform matrix or the cam attributes
	// made this way so that it can be a compatible node3d and be nested within other nodes
	if (from_transform) {
		_camera._eye.x = _transform[3][0];
		_camera._eye.y = _transform[3][1];
		_camera._eye.z = _transform[3][2];
	}
	else {
		_transform[3][0] = _camera._eye.x;
		_transform[3][1] = _camera._eye.y;
		_transform[3][2] = _camera._eye.z;
	}
	_camera.viewMatrix = glm::lookAt(_camera._eye, _camera._centre, _camera._up);
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
	_camera._eye = eye;
	_camera._centre = center;
	_camera._up = up;
	_transform = glm::translate(glm::mat4(1.0f), eye);
	updateCamera();
}

float Astra::CameraController::getNear() const
{
	return _camera._near;
}

float Astra::CameraController::fetFar() const
{
	return _camera._far;
}

float Astra::CameraController::getFov() const
{
	return _camera._fov;
}

glm::vec3 Astra::CameraController::getEye() const
{
	return _camera._eye;
}

glm::vec3 Astra::CameraController::getUp() const
{
	return _camera._up;
}

glm::vec3 Astra::CameraController::getCentre() const
{
	return _camera._centre;
}

float& Astra::CameraController::getNear()
{
	return _camera._near;
}

float& Astra::CameraController::fetFar()
{
	return _camera._far;
}

float& Astra::CameraController::getFov()
{
	return _camera._fov;
}

glm::vec3& Astra::CameraController::getEye()
{
	return _camera._eye;
}

glm::vec3& Astra::CameraController::getUp()
{
	return _camera._up;
}

glm::vec3& Astra::CameraController::getCentre()
{
	return _camera._centre;
}

void Astra::CameraController::setNear(float n)
{
	_camera._near = n;
}

void Astra::CameraController::setFar(float f)
{
	_camera._far = f;
}

void Astra::CameraController::setFov(float f)
{
	_camera._fov = f;
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

void Astra::OrbitCameraController::orbit(float dx, float dy)
{
	//y *= -1;
	//float current_rotation[3];
	//glm::extractEulerAngleXYZ(_transform, current_rotation[0], current_rotation[1], current_rotation[2]);
	//std::cout << "Rotation: " << current_rotation[0] << " - " << current_rotation[1] << " - " << current_rotation[2] << std::endl;
	//
	//_transform = glm::mat4(1.0f);

	//// rotating around Y axis
	//rotate(glm::vec3(0, 1, 0), x * 0.01);

	////// rotation around X axis (needs to be clamped)
	///*float verticalRotation = rotationX;
	//float giro = verticalRotation;
	//verticalRotation = verticalRotation + y * 0.01f;
	//verticalRotation = glm::clamp(verticalRotation, minXRotation, maxXRotation);
	//giro = verticalRotation - giro;
	//rotationX += giro;*/
	//rotate(glm::vec3(1, 0, 0), y * 0.01);
	////if (y != 0) {
	////	std::cout << "Current Rotation: " << current_rotation[0] << std::endl;
	////	std::cout << "Rotation to apply: " << giro << std::endl;
	////	std::cout << "Y input: " << y << std::endl;
	////}

	//// establishing new position from rotation
	//auto newpos = _transform * glm::vec4(_camera.eye - _camera.centre, 1.0f);
	//_transform[3] = newpos;
	//updateCamera();


	//glm::mat4 TR = glm::transpose(glm::translate(glm::mat4(1.0f), _camera._centre));
	//glm::mat4 TRI = glm::transpose(glm::translate(glm::mat4(1.0f), -_camera._centre));
	//glm::mat4 R;
	//glm::mat4 T;


	//R = glm::rotate(glm::mat4(1.0f), glm::radians(y*0.5f), glm::normalize(glm::cross(_camera._up, _camera._centre - _camera._eye)));
	//T = TR * R;
	//T = T * TRI;
	//_transform = T * _transform;

	//R = glm::transpose((glm::rotate(glm::mat4(1.0f), glm::radians(x * 0.5f), glm::vec3(0.0, 1.0, 0.0))));
	//T = TR * R;
	//T = T * TRI;
	//_transform = T * _transform;

	//
	//updateCamera();

	if (dx == 0 && dy == 0)
		return;

	// Full width will do a full turn
	dx *= _sens;// * glm::two_pi<float>();
	dy *= _sens;// *glm::two_pi<float>();

	// Get the camera
	glm::vec3 origin (_camera._centre);
	glm::vec3 position(_camera._eye);

	// Get the length of sight
	glm::vec3 centerToEye(position - origin);
	float     radius = glm::length(centerToEye);
	centerToEye = glm::normalize(centerToEye);
	glm::vec3 axe_z = centerToEye;

	// Find the rotation around the UP axis (Y)
	glm::mat4 rot_y = glm::rotate(glm::mat4(1), -dx, _camera._up);

	// Apply the (Y) rotation to the eye-center vector
	centerToEye = rot_y * glm::vec4(centerToEye, 0);

	// Find the rotation around the X vector: cross between eye-center and up (X)
	glm::vec3 axe_x = glm::normalize(glm::cross(_camera._up, axe_z));
	glm::mat4 rot_x = glm::rotate(glm::mat4(1), -dy, axe_x);

	// Apply the (X) rotation to the eye-center vector
	glm::vec3 vect_rot = rot_x * glm::vec4(centerToEye, 0);

	if (glm::sign(vect_rot.x) == glm::sign(centerToEye.x))
		centerToEye = vect_rot;

	// Make the vector as long as it was originally
	centerToEye *= radius;

	// Finding the new position
	glm::vec3 newPosition = centerToEye + origin;
	
	_camera._eye = newPosition; 

	updateCamera(false);
	

}

void Astra::OrbitCameraController::zoom(float increment)
{
	_camera._fov = glm::clamp(_camera._fov + increment * 0.1f, 10.0f, 120.0f);
}

void Astra::OrbitCameraController::pan(float dx, float dy)
{

	glm::vec3 z(_camera._eye - _camera._centre);
	z = glm::normalize(z);
	glm::vec3 x = glm::cross(_camera._up, z);
	glm::vec3 y = glm::cross(z, x);
	x = glm::normalize(x);
	y = glm::normalize(y);

	glm::vec3 panVector = (-dx * _sens * x + dy * y * _sens);
	_camera._eye += panVector;
	_camera._centre += panVector;

	updateCamera(false);
}
