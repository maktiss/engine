#pragma once

#include <spdlog/spdlog.h>

#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <vector>


namespace Engine {
template <typename DerivedManager, typename... ScriptBaseTypes>
class ScriptManagerBase {
public:
	class Handle {
	private:
		uint32_t index {};

	public:
		Handle(uint32_t index) : index(index) {
		}

		inline auto getIndex() const {
			return index;
		}
	};


private:
	struct ScriptInfo {
		uint32_t typeIndex {};
		uint32_t localIndex {};
	};


private:
	static std::tuple<std::vector<std::shared_ptr<ScriptBaseTypes>>...> scripts;

	static std::map<std::string, ScriptInfo> scriptInfos;


public:
	static int init() {
		// Remove all scripts
		((std::get<std::vector<std::shared_ptr<ScriptBaseTypes>>>(scripts) = {}), ...);
		scriptInfos = {};

		// Register fallback scripts
		((registerScript<ScriptBaseTypes>()), ...);

		return 0;
	}


	template <typename ScriptType>
	static void registerScript() {
		((registerScriptImpl<ScriptType, ScriptBaseTypes>()), ...);
	}


	static inline Handle getScriptHandle(std::string name) {
		auto index = scriptInfos[name].localIndex;
		if (index == 0) {
			spdlog::error("Requested non-existing script: {}", name);
		}
		return Handle(index);
	}

	template <typename ScriptBaseType>
	static inline auto& getScript(Handle handle) {
		return std::get<std::vector<std::shared_ptr<ScriptBaseType>>>(scripts)[handle.getIndex()];
	}


private:
	template <typename ScriptType, typename ScriptBaseType>
	static void registerScriptImpl() {
		if (std::is_base_of<ScriptBaseType, ScriptType>::value) {
			auto script = std::static_pointer_cast<ScriptBaseType>(std::make_shared<ScriptType>());

			auto iter = scriptInfos.find(script->getScriptName());

			if (iter != scriptInfos.end()) {
				spdlog::error("Attempted to register a script with existing name: {}", script->getScriptName());
				return;
			}

			auto& scriptArray = std::get<std::vector<std::shared_ptr<ScriptBaseType>>>(scripts);
			scriptArray.push_back(script);

			constexpr auto typeIndex			 = getScriptTypeIndex<ScriptBaseType>();
			uint32_t index						 = scriptArray.size() - 1;
			scriptInfos[script->getScriptName()] = { typeIndex, index };
		}
	}

	template <typename ScriptBaseType>
	static constexpr uint32_t getScriptTypeIndex() {
		return getScriptTypeIndexImpl<ScriptBaseType>(std::make_index_sequence<sizeof...(ScriptBaseTypes)>());
	}

	template <typename ScriptBaseType, std::size_t... Indices>
	static constexpr uint32_t getScriptTypeIndexImpl(std::index_sequence<Indices...>) {
		return ((std::is_same<typename std::tuple_element<Indices, std::tuple<ScriptBaseTypes...>>::type,
							  ScriptBaseType>::value
					 ? Indices
					 : 0) +
				...);
	}


protected:
	ScriptManagerBase() {};
};

template <typename DerivedManager, typename... ScriptBaseTypes>
std::tuple<std::vector<std::shared_ptr<ScriptBaseTypes>>...>
	ScriptManagerBase<DerivedManager, ScriptBaseTypes...>::scripts {};

template <typename DerivedManager, typename... ScriptBaseTypes>
std::map<std::string, typename ScriptManagerBase<DerivedManager, ScriptBaseTypes...>::ScriptInfo>
	ScriptManagerBase<DerivedManager, ScriptBaseTypes...>::scriptInfos {};
} // namespace Engine