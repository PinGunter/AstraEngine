#include <App.h>
#include <nvpsystem.hpp>
#include <Utils.h>
#include <nvh/fileoperations.hpp>

int main(int argc, char** argv) {
	// Device Initialization
	Astra::DeviceCreateInfo createInfo;
	Astra::Device::getInstance().initDevice(createInfo);
	const auto& device = Astra::Device::getInstance();

	// App creation
	Astra::DefaultApp app;
	Astra::SceneRT * scene = new Astra::SceneRT();
	Astra::Renderer* renderer = new Astra::Renderer();

	// Renderer creation
	renderer->init();
	// Scene creation
	Astra::Camera cam;
	Astra::CameraController* camera = new Astra::OrbitCameraController(cam);
	// Setup camera
	auto windowSize = device.getWindowSize();
	camera->setWindowSize(windowSize[0], windowSize[1]);
	camera->setLookAt(glm::vec3(5.0f), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

	scene->setCamera(camera);

	app.init({ scene }, renderer);
	app.setupCallbacks(Astra::Device::getInstance().getWindow());

	try {
		app.run();
	}
	catch (...) {
		app.destroy();
		Astra::Log("Exception ocurred", Astra::LOG_LEVELS::ERR);
	}

	delete camera;
	delete renderer;
	delete scene;
}

