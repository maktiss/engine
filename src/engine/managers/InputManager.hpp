#pragma once

#include <glm/glm.hpp>

#include <cstdint>
#include <map>
#include <vector>


namespace Engine::Managers {
class InputManager {
public:
	enum class KeyAction {
		UNDEFINED = 0,
		CAMERA_MOVE_FORWARD,
		CAMERA_MOVE_BACKWARD,
		CAMERA_MOVE_LEFT,
		CAMERA_MOVE_RIGHT,
		CAMERA_MOVE_UP,
		CAMERA_MOVE_DOWN,
	};

	enum class AxisAction {
		UNDEFINED = 0,
		CAMERA_MOVE_FORWARD,
		CAMERA_MOVE_BACKWARD,
		CAMERA_MOVE_LEFT,
		CAMERA_MOVE_RIGHT,
	};

	enum KeyActionState {
		PRESSED		 = 1 << 0,
		PRESSED_ONCE = 1 << 1,
		RELEASED	 = 1 << 2,
	};

private:
	static std::vector<uint> actionStates;

	static std::vector<glm::vec3> actionAxes;

public:
	static int init() {
		// TODO: better solution
		actionStates.resize(7);
		actionAxes.resize(5);

		return 0;
	}


	static inline uint getKeyActionState(KeyAction action) {
		return actionStates[static_cast<int>(action)];
	}


	static inline bool isKeyActionStatePressed(KeyAction action) {
		return getKeyActionState(action) & KeyActionState::PRESSED;
	}

	static inline bool isKeyActionStatePressedOnce(KeyAction action) {
		return getKeyActionState(action) & KeyActionState::PRESSED_ONCE;
	}

	static inline bool isKeyActionStateReleased(KeyAction action) {
		return getKeyActionState(action) & KeyActionState::RELEASED;
	}


	static inline glm::vec3 getActionAxis(AxisAction action) {
		return actionAxes[static_cast<int>(action)];
	}


	static inline void setKeyActionPressed(KeyAction action) {
		auto& state = actionStates[static_cast<int>(action)];
		
		if (state & KeyActionState::PRESSED) {
			state &= ~KeyActionState::PRESSED_ONCE;
		} else {
			state |= KeyActionState::PRESSED_ONCE;
		}

		state |= KeyActionState::PRESSED;
	}

	static inline void setKeyActionReleased(KeyAction action) {
		auto& state = actionStates[static_cast<int>(action)];
		
		if (state & KeyActionState::RELEASED) {
			state = 0;
		} else {
			state = KeyActionState::RELEASED;
		}
	}


private:
	InputManager() {};
};
} // namespace Engine::Managers