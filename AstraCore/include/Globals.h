#pragma once
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <nvp/nvpsystem.hpp>

namespace Astra {

	static std::vector<std::string> defaultSearchPaths = {
		NVPSystem::exePath() + PROJECT_RELDIRECTORY,
		NVPSystem::exePath() + PROJECT_RELDIRECTORY "..",
		std::string(PROJECT_NAME),
	};

	enum PipelineBindPoints {
		Graphics = VK_PIPELINE_BIND_POINT_GRAPHICS,
		Compute = VK_PIPELINE_BIND_POINT_COMPUTE,
		RayTracing = VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR
	};

	enum AppStatus {
		Created,
		Running,
		Destroyed
	};

	// this enums just match the GLFW ones
	enum MouseButtons
	{
		Left = GLFW_MOUSE_BUTTON_1,
		Right = GLFW_MOUSE_BUTTON_2,
		Middle = GLFW_MOUSE_BUTTON_3,
		SIZE_MOUSEBUTTONS
	};

	enum Keys
	{
		Key_Space = GLFW_KEY_SPACE,
		Key_Apostrophe = GLFW_KEY_APOSTROPHE,
		Key_Comma = GLFW_KEY_COMMA,
		Key_Minus = GLFW_KEY_MINUS,
		Key_Period = GLFW_KEY_PERIOD,
		Key_Slash = GLFW_KEY_SLASH,
		Key_0 = GLFW_KEY_0,
		Key_1 = GLFW_KEY_1,
		Key_2 = GLFW_KEY_2,
		Key_3 = GLFW_KEY_3,
		Key_4 = GLFW_KEY_4,
		Key_5 = GLFW_KEY_5,
		Key_6 = GLFW_KEY_6,
		Key_7 = GLFW_KEY_7,
		Key_8 = GLFW_KEY_8,
		Key_9 = GLFW_KEY_9,
		Key_Semicolon = GLFW_KEY_SEMICOLON,
		Key_Equal = GLFW_KEY_EQUAL,
		Key_A = GLFW_KEY_A,
		Key_B = GLFW_KEY_B,
		Key_C = GLFW_KEY_C,
		Key_D = GLFW_KEY_D,
		Key_E = GLFW_KEY_E,
		Key_F = GLFW_KEY_F,
		Key_G = GLFW_KEY_G,
		Key_H = GLFW_KEY_H,
		Key_I = GLFW_KEY_I,
		Key_J = GLFW_KEY_J,
		Key_K = GLFW_KEY_K,
		Key_L = GLFW_KEY_L,
		Key_M = GLFW_KEY_M,
		Key_N = GLFW_KEY_N,
		Key_O = GLFW_KEY_O,
		Key_P = GLFW_KEY_P,
		Key_Q = GLFW_KEY_Q,
		Key_R = GLFW_KEY_R,
		Key_S = GLFW_KEY_S,
		Key_T = GLFW_KEY_T,
		Key_U = GLFW_KEY_U,
		Key_V = GLFW_KEY_V,
		Key_W = GLFW_KEY_W,
		Key_X = GLFW_KEY_X,
		Key_Y = GLFW_KEY_Y,
		Key_Z = GLFW_KEY_Z,
		Key_LeftBracket = GLFW_KEY_LEFT_BRACKET,
		Key_KeyBackslash = GLFW_KEY_BACKSLASH,
		Key_RightBracket = GLFW_KEY_RIGHT_BRACKET,
		Key_GraveAccent = GLFW_KEY_GRAVE_ACCENT,
		Key_Escape = GLFW_KEY_ESCAPE,
		Key_Enter = GLFW_KEY_ENTER,
		Key_Tab = GLFW_KEY_TAB,
		Key_Backspace = GLFW_KEY_BACKSPACE,
		Key_Insert = GLFW_KEY_INSERT,
		Key_Delete = GLFW_KEY_DELETE,
		Key_Right = GLFW_KEY_RIGHT,
		Key_Left = GLFW_KEY_LEFT,
		Key_Down = GLFW_KEY_DOWN,
		Key_Up = GLFW_KEY_UP,
		Key_PageUp = GLFW_KEY_PAGE_UP,
		Key_PageDown = GLFW_KEY_PAGE_DOWN,
		Key_Home = GLFW_KEY_HOME,
		Key_End = GLFW_KEY_END,
		Key_CapsLock = GLFW_KEY_CAPS_LOCK,
		Key_ScrollLock = GLFW_KEY_SCROLL_LOCK,
		Key_NumLock = GLFW_KEY_NUM_LOCK,
		Key_PrntScrn = GLFW_KEY_PRINT_SCREEN,
		Key_Pause = GLFW_KEY_PAUSE,
		Key_F1 = GLFW_KEY_F1,
		Key_F2 = GLFW_KEY_F2,
		Key_F3 = GLFW_KEY_F3,
		Key_F4 = GLFW_KEY_F4,
		Key_F5 = GLFW_KEY_F5,
		Key_F6 = GLFW_KEY_F6,
		Key_F7 = GLFW_KEY_F7,
		Key_F8 = GLFW_KEY_F8,
		Key_F9 = GLFW_KEY_F9,
		Key_F10 = GLFW_KEY_F10,
		Key_F11 = GLFW_KEY_F11,
		Key_F12 = GLFW_KEY_F12,
		Key_LeftShift = GLFW_KEY_LEFT_SHIFT,
		Key_LeftControl = GLFW_KEY_LEFT_CONTROL,
		Key_LeftAlt = GLFW_KEY_LEFT_ALT,
		Key_LeftSuper = GLFW_KEY_LEFT_SUPER,
		Key_RightShift = GLFW_KEY_RIGHT_SHIFT,
		Key_RightControl = GLFW_KEY_RIGHT_CONTROL,
		Key_RightAlt = GLFW_KEY_RIGHT_ALT,
		Key_RightSuper = GLFW_KEY_RIGHT_SUPER,
		SIZE_KEYMAP
	};
}