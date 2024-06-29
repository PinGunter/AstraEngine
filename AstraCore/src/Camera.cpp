#include <Camera.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <iostream>
#include <Globals.h>
#include <Utils.h>
#include <InputManager.h>

Astra::CameraController::CameraController(Camera& cam) : _camera(cam)
{
	auto size = AstraDevice.getWindowSize();
	_width = size[0];
	_height = size[1];
	updateCamera();
}

void Astra::CameraController::updateCamera()
{
	_camera.viewMatrix = glm::lookAt(_camera.eye, _camera.centre, _camera.up);
}

const glm::mat4& Astra::CameraController::getViewMatrix() const
{
	return _camera.viewMatrix;
}

glm::mat4 Astra::CameraController::getProjectionMatrix() const
{
	assert(_height != 0.0 && _width != 0.0);
	float aspectRatio = _width / static_cast<float>(_height);
	glm::mat4 proj = glm::perspectiveRH_ZO(glm::radians(_camera.fov), aspectRatio, _camera.nearPlane, _camera.farPlane);
	proj[1][1] *= -1; // vulkan shenanigans ;)
	return proj;
}

void Astra::CameraController::setLookAt(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up)
{
	_camera.eye = eye;
	_camera.centre = center;
	_camera.up = up;
	updateCamera();
}

float Astra::CameraController::getNear() const
{
	return _camera.nearPlane;
}

float Astra::CameraController::fetFar() const
{
	return _camera.farPlane;
}

float Astra::CameraController::getFov() const
{
	return _camera.fov;
}

const glm::vec3& Astra::CameraController::getEye() const
{
	return _camera.eye;
}

const glm::vec3& Astra::CameraController::getUp() const
{
	return _camera.up;
}

const glm::vec3& Astra::CameraController::getCentre() const
{
	return _camera.centre;
}

Astra::Camera Astra::CameraController::getCamera() const
{
	return _camera;
}

float& Astra::CameraController::getNearRef()
{
	return _camera.nearPlane;
}

float& Astra::CameraController::fetFarRef()
{
	return _camera.farPlane;
}

float& Astra::CameraController::getFovRef()
{
	return _camera.fov;
}

glm::vec3& Astra::CameraController::getEyeRef()
{
	return _camera.eye;
}

glm::vec3& Astra::CameraController::getUpRef()
{
	return _camera.up;
}

glm::vec3& Astra::CameraController::getCentreRef()
{
	return _camera.centre;
}

void Astra::CameraController::setNear(float n)
{
	_camera.nearPlane = n;
}

void Astra::CameraController::setFar(float f)
{
	_camera.farPlane = f;
}

void Astra::CameraController::setFov(float f)
{
	_camera.fov = f;
}

bool Astra::CameraController::update()
{
	// update camera params
	updateCamera();
	return true;
}

CameraUniform Astra::CameraController::getCameraUniform()
{
	CameraUniform hostUBO = {};
	hostUBO.viewProj = getProjectionMatrix() * getViewMatrix();
	hostUBO.viewInverse = glm::inverse(getViewMatrix());
	hostUBO.projInverse = glm::inverse(getProjectionMatrix());

	return hostUBO;
}

Astra::FreeCameraController::FreeCameraController(Camera& cam) : CameraController(cam)
{
	_sens = 0.005f;
}

bool Astra::FreeCameraController::update()
{
	CameraController::update();

	move(
		Input.keyPressed(Key_W),
		Input.keyPressed(Key_S),
		Input.keyPressed(Key_D),
		Input.keyPressed(Key_A),
		Input.keyPressed(Key_Q),
		Input.keyPressed(Key_E),
		(Input.keyPressed(Key_LeftControl) ? _speed * 10 : _speed));

	if (Input.mouseClick(Right))
	{
		Input.captureMouse();

		glm::ivec2 delta = Input.getMouseDelta();
		float dx = delta.x * _sens;
		float dy = delta.y * _sens;
		rotate(dx, dy);
	}
	else
	{
		Input.freeMouse();
	}

	updateCamera();
	return true;
}

void Astra::FreeCameraController::move(bool front, bool back, bool right, bool left, bool up, bool down, float speed)
{
	glm::vec3 frontBack = _camera.centre - _camera.eye;
	float length = glm::length(frontBack);
	glm::vec3 direction = glm::normalize(_camera.centre - _camera.eye);
	glm::vec3 rightLeft = glm::normalize(glm::cross(frontBack, _camera.up));

	float zSpeed = 0, xSpeed = 0, ySpeed = 0;

	/*
		The following sequence of ifs could be refactored to this:

		xSpeed = right ? speed : left ? -speed : 0;
		ySpeed = up ? speed : down ? -speed : 0;
		zSpeed = front ? speed : back ? -speed : 0;

		However, this sequence is left instead of nesting ternary operators
		so that it is easier to read
	*/
	if (front)
	{
		zSpeed += speed;
	}
	if (back)
	{
		zSpeed -= speed;
	}
	if (right)
	{
		xSpeed += speed;
	}
	if (left)
	{
		xSpeed -= speed;
	}
	if (up)
	{
		ySpeed += speed;
	}
	if (down)
	{
		ySpeed -= speed;
	}

	_camera.eye = _camera.eye + direction * zSpeed + rightLeft * xSpeed + _camera.up * ySpeed;
	_camera.centre = _camera.eye + direction * length;
}

void Astra::FreeCameraController::rotate(float dx, float dy)
{

	// Get the length of sight
	glm::vec3 eyeToCenter(_camera.centre - _camera.eye);
	float radius = glm::length(eyeToCenter);
	eyeToCenter = glm::normalize(eyeToCenter);
	glm::vec3 direction = eyeToCenter;

	// Find the rotation around the UP axis (Y)
	glm::mat4 rot_y = glm::rotate(glm::mat4(1), -dx, _camera.up);

	// Apply the (Y) rotation to the eye-center vector
	eyeToCenter = rot_y * glm::vec4(eyeToCenter, 0);

	// Find the rotation around the X vector: cross between eye-center and up (X)
	glm::vec3 axe_x = glm::normalize(glm::cross(_camera.up, direction));
	glm::mat4 rot_x = glm::rotate(glm::mat4(1), dy, axe_x);

	// Apply the (X) rotation to the eye-center vector
	glm::vec3 vect_rot = rot_x * glm::vec4(eyeToCenter, 0);

	if (glm::sign(vect_rot.x) == glm::sign(eyeToCenter.x))
		eyeToCenter = vect_rot;

	// Make the vector as long as it was originally
	eyeToCenter *= radius;

	// Finding the new position
	glm::vec3 newCentre = eyeToCenter + _camera.eye;

	_camera.centre = newCentre;
}

Astra::OrbitCameraController::OrbitCameraController(Camera& cam) : CameraController(cam)
{
}

bool Astra::OrbitCameraController::update()
{
	glm::vec2 delta = Input.getMouseDelta();
	float wheel = Input.getMouseWheel().y;
	if (Input.mouseClick(Right))
	{
		if (Input.keyPressed(Key_LeftControl))
		{
			zoom(delta.y);
		}
		else
		{
			orbit(delta.x, delta.y);
		}
	}
	else if (Input.mouseClick(Middle))
	{
		pan(delta.x, delta.y);
	}

	zoom(wheel * 10.0f);

	updateCamera();
	return true;
}

void Astra::OrbitCameraController::orbit(float dx, float dy)
{

	if (dx == 0 && dy == 0)
		return;

	dx *= _sens;
	dy *= _sens;

	// Get the length of sight
	glm::vec3 centerToEye(_camera.eye - _camera.centre);
	float radius = glm::length(centerToEye);
	centerToEye = glm::normalize(centerToEye);
	glm::vec3 direction = centerToEye;

	// Find the rotation around the UP axis (Y)
	glm::mat4 rot_y = glm::rotate(glm::mat4(1), -dx, _camera.up);

	// Apply the (Y) rotation to the eye-center vector
	centerToEye = rot_y * glm::vec4(centerToEye, 0);

	// Find the rotation around the X vector: cross between eye-center and up (X)
	glm::vec3 axe_x = glm::normalize(glm::cross(_camera.up, direction));
	glm::mat4 rot_x = glm::rotate(glm::mat4(1), -dy, axe_x);

	// Apply the (X) rotation to the eye-center vector
	glm::vec3 vect_rot = rot_x * glm::vec4(centerToEye, 0);

	if (glm::sign(vect_rot.x) == glm::sign(centerToEye.x))
		centerToEye = vect_rot;

	// Make the vector as long as it was originally
	centerToEye *= radius;

	// Finding the new position
	glm::vec3 newPosition = centerToEye + _camera.centre;

	_camera.eye = newPosition;
}

void Astra::OrbitCameraController::pan(float dx, float dy)
{
	glm::vec3 direction = glm::normalize(_camera.eye - _camera.centre);
	glm::vec3 horizontal = glm::normalize(glm::cross(_camera.up, direction));
	glm::vec3 vertical = glm::normalize(glm::cross(direction, horizontal));

	glm::vec3 panVector = (_sens * (-dx * horizontal + dy * vertical));
	_camera.eye += panVector;
	_camera.centre += panVector;
}

void Astra::OrbitCameraController::zoom(float increment)
{
	_camera.fov = glm::clamp(_camera.fov + increment * 0.1f, 10.0f, 120.0f);
}
