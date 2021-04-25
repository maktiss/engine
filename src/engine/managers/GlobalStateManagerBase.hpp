#pragma once

#include <tuple>


namespace Engine {
template <typename... States>
class GlobalStateManagerBase {
private:
	static std::tuple<States...> states;

	// Updated states that become main on update
	static std::tuple<States...> nextStates;

public:
	template <typename State>
	static const auto& get() {
		return std::get<State>(states);
	}

	template <typename State>
	static auto& getWritable() {
		return std::get<State>(nextStates);
	}


	static int update() {
		states = nextStates;

		return 0;
	}
};


template <typename... States>
std::tuple<States...> GlobalStateManagerBase<States...>::states {};

template <typename... States>
std::tuple<States...> GlobalStateManagerBase<States...>::nextStates {};

} // namespace Engine