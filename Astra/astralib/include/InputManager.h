#pragma once
// Work in Progress!
namespace Astra {
	class InputManager {
	private:
		InputManager();
		~InputManager();
	public:
		static InputManager& getInstance() {
			static InputManager instance;
			return instance;
		}
		InputManager(const InputManager&) = delete;
		InputManager& operator=(const InputManager&) = delete;
	};
}