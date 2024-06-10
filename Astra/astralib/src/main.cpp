#include <App.h>
#include <nvpsystem.hpp>
#include <Utils.h>
#include <nvh/fileoperations.hpp>
#include <glm/gtc/matrix_transform.hpp>


// search paths for finding files
std::vector<std::string> defaultSearchPaths = {
	NVPSystem::exePath() + PROJECT_RELDIRECTORY,
	NVPSystem::exePath() + PROJECT_RELDIRECTORY "..",
	std::string(PROJECT_NAME),
};

int main(int argc, char** argv) {
	// Device Initialization
	Astra::DeviceCreateInfo createInfo{};
	AstraDevice.initDevice(createInfo);

	// App creation
	Astra::DefaultApp app;
	Astra::SceneRT* scene = new Astra::SceneRT();
	Astra::Renderer* renderer = new Astra::Renderer();
	Astra::GuiController* gui = new Astra::BasiGui();

	// Renderer creation
	renderer->init();

	// Scene creation
	Astra::Camera cam;
	Astra::CameraController* camera = new Astra::OrbitCameraController(cam);
	Astra::Light* pointLight = new Astra::PointLight(glm::vec3(1.0f), 60.0f);
	pointLight->translate(glm::vec3(10, 15, 20));
	// Setup camera
	auto windowSize = AstraDevice.getWindowSize();
	camera->setWindowSize(windowSize[0], windowSize[1]);
	camera->setLookAt(glm::vec3(5.0f), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

	scene->setCamera(camera);
	scene->addLight(pointLight);

	scene->loadModel(nvh::findFile("media/scenes/mono2.obj", defaultSearchPaths, true));
	scene->loadModel(nvh::findFile("media/scenes/plane2.obj", defaultSearchPaths, true), glm::translate(glm::mat4(1.0f), glm::vec3(0, -1, 0)));

	app.init({ scene }, renderer, gui);

	try {
		app.run();
	}
	catch (...) {
		app.destroy();
		Astra::Log("Exception ocurred", Astra::LOG_LEVELS::ERR);
	}

	AstraDevice.destroy();

	delete camera;
	delete renderer;
	delete scene;
	delete pointLight;
	delete gui;
}

