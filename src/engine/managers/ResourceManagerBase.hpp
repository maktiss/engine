#pragma once

#include <spdlog/spdlog.h>

#include <stdint.h>
#include <tuple>
#include <unordered_map>
#include <vector>


namespace Engine::Managers {
template <typename DerivedManager, typename... ManageableTypes>
class ResourceManagerBase {
public:
	class Handle {
		uint32_t index = 0;
		uint32_t id	   = 0;

	public:
		Handle() {};

		Handle(uint32_t id, uint32_t index) : id(id), index(index) {
			incrementReferenceCounter(index);
		}

		Handle(const Handle& handle) : id(handle.id), index(handle.index) {
			incrementReferenceCounter(index);
		}

		~Handle() {
			decrementReferenceCounter(index);
		}

		inline uint32_t getIndex() {
			return index;
		}

		inline uint32_t getId() {
			return id;
		}
	};

private:
	struct ObjectInfo {
		uint32_t index = 0;
		bool isLoaded  = false;

		// position within tuple of vertors
		uint32_t typeIndex	= 0;
		uint32_t localIndex = 0;
	};

private:
	static std::tuple<std::vector<ManageableTypes>...> objects;

	static std::unordered_map<uint32_t, ObjectInfo> objectInfos;

	static std::vector<int32_t> referenceCounts;

public:
	static int init() {
		// fallback object
		createObject(0);

		// TODO: load object table

		return 0;
	}

	static Handle getHandle(uint32_t id) {
		const auto& objectInfo = objectInfos[id];

		Handle handle(id, objectInfo.index);

		if (!objectInfo.isLoaded) {
			load(id);
			DerivedManager::postLoad(handle);
		}

		return handle;
	}

	static void load(uint32_t id) {
	}

	static Handle createObject(uint32_t typeIndex) {
		auto id = getNextFreeId();

		auto newSize = createObjectImpl(typeIndex, std::make_index_sequence<getTypeCount()>());

		if (newSize == 0) {
			// TODO: error: typeIndex out of range
		}

		objectInfos[id] = { referenceCounts.size(), true, typeIndex, newSize - 1};

		referenceCounts.push_back(0);

		auto handle = getHandle(id);
		DerivedManager::postCreate(handle);

		return handle;
	}


	static inline uint32_t getNumObjects() {
		return objectInfos.size();
	}

	static inline uint32_t getTypeIndex(Handle handle) {
		return objectInfos[handle.getId()].typeIndex;
	}

	static constexpr uint32_t getTypeCount() {
		return sizeof...(ManageableTypes);
	}

	template <typename Func>
	static void apply(Handle handle, Func&& func) {
		applyImpl(
			objectInfos[handle.getId()], func, std::make_index_sequence<getTypeCount()>());
	}

	static constexpr auto getTypeTuple() {
		return std::tuple<ManageableTypes...>();
	}

	template <typename Type>
	static constexpr uint32_t getTypeIndex() {
		return getTypeIndexImpl<Type>(std::make_index_sequence<sizeof...(ManageableTypes)>());
	}

	template <typename Type, std::size_t... Indices>
	static constexpr uint32_t getTypeIndexImpl(std::index_sequence<Indices...>) {
		return ((std::is_same<typename std::tuple_element<Indices, std::tuple<ManageableTypes...>>::type,
							  Type>::value
					 ? Indices
					 : 0) +
				...);
	}

private:
	// returns size of a vector a new object have been created in
	template <std::size_t... Indices>
	static uint32_t createObjectImpl(uint32_t typeIndex, std::index_sequence<Indices...>) {
		return (createObjectImpl<Indices>(typeIndex) + ...);
	}

	template <std::size_t Index>
	static uint32_t createObjectImpl(uint32_t typeIndex) {
		if (Index == typeIndex) {
			std::get<Index>(objects).push_back({});
			return std::get<Index>(objects).size();
		}
		return 0;
	}


	template <typename Func, std::size_t... Indices>
	static void applyImpl(ObjectInfo objectInfo, Func&& func, std::index_sequence<Indices...>) {
		(applyImpl<Indices>(objectInfo, func), ...);
	}

	template <std::size_t Index, typename Func>
	static void applyImpl(ObjectInfo objectInfo, Func&& func) {
		if (Index == objectInfo.typeIndex) {
			func(std::get<Index>(objects)[objectInfo.localIndex]);
		}
	}

protected:
	ResourceManagerBase() {};

private:
	static uint32_t getNextFreeId() {
		uint32_t id = 0;
		while (true) {
			if (objectInfos.find(id) == objectInfos.end()) {
				return id;
			}
			id++;
		}
		return 0;
	}

	static inline void incrementReferenceCounter(uint32_t index) {
		referenceCounts[index]++;
	}

	static inline void decrementReferenceCounter(uint32_t index) {
		referenceCounts[index]--;
	}
};

template <typename DerivedManager, typename... ManageableTypes>
std::tuple<std::vector<ManageableTypes>...> ResourceManagerBase<DerivedManager, ManageableTypes...>::objects;

template <typename DerivedManager, typename... ManageableTypes>
std::unordered_map<uint32_t, typename ResourceManagerBase<DerivedManager, ManageableTypes...>::ObjectInfo>
	ResourceManagerBase<DerivedManager, ManageableTypes...>::objectInfos;

template <typename DerivedManager, typename... ManageableTypes>
std::vector<int32_t> ResourceManagerBase<DerivedManager, ManageableTypes...>::referenceCounts;

} // namespace Engine::Managers