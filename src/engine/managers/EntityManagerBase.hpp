#pragma once

#include <spdlog/spdlog.h>

#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <string>
#include <tuple>
#include <unordered_map>
#include <variant>
#include <vector>


namespace Engine {
template <typename... ComponentTypes>
class EntityManagerBase {
public:
	class Handle {
	private:
		uint32_t index = 0;

	public:
		Handle() {
		}

		Handle(uint32_t index) : index(index) {
		}

		inline uint32_t getIndex() const {
			return index;
		}

		template <typename ComponentType>
		inline ComponentType& getComponent() {
			return EntityManagerBase::getComponent<ComponentType>(index);
		}

		template <typename... RequiredComponentTypes, typename Func>
		inline void apply(Func&& func) {
			EntityManagerBase::apply<RequiredComponentTypes...>(index, func);
		}
	};

	using Ranges = std::vector<std::pair<uint32_t, uint32_t>>;


private:
	static std::tuple<std::vector<ComponentTypes>...> componentArrays;

	// Entity name to index mapping
	static std::unordered_map<std::string, uint32_t> entityIndicesMap;

	// Index to name mapping
	static std::vector<std::string> entityNames;

	static std::array<Ranges, sizeof...(ComponentTypes)> componentRanges;


public:
	static int init() {
		// create fallback entity, which always has every component
		reserve(1);
		for (auto& ranges : componentRanges) {
			ranges.push_back({ 0, 1 });
		}

		return 0;
	}

	static inline uint32_t getEntityCount() {
		return std::get<0>(componentArrays).size();
	}


	template <typename... RequiredComponentTypes>
	static Handle createEntity(std::string name) {
		uint32_t index = getEntityCount();
		reserve(index + 1);

		auto signature = getSignature<RequiredComponentTypes...>();
		while (true) {
			auto leftSignature = getSignature(index - 1);

			if (signature <= leftSignature) {
				fixRangesAtPosition(index, signature);
				break;
			} else {
				uint32_t leftEdge	= 0;
				auto leftRangeEdges = getLeftRangeEdges(index - 1);

				for (auto leftRangeEdge : leftRangeEdges) {
					leftEdge = std::max(leftRangeEdge, leftEdge);
				}

				moveEntity(leftEdge, index);

				fixRangesAtPosition(index, leftSignature);

				index = leftEdge;
			}
		}

		// check if position is correct
		assert((index == 0) || (signature <= getSignature(index - 1)));
		assert((index == (getEntityCount() - 1)) || (signature > getSignature(index + 1)));

		resetEntity(index);


		std::string originalName = name;

		auto suffix = 1;

		while (entityIndicesMap.find(name) != entityIndicesMap.end()) {
			auto foundPos = name.find_last_of("_");

			if (foundPos != std::string::npos) {
				auto indexStr = name.substr(foundPos + 1);

				if (indexStr.find_first_not_of("1234567890") == std::string::npos) {
					suffix = std::stoi(indexStr) + 1;
				}
			}

			name = name.substr(0, foundPos) + "_" + std::to_string(suffix);
		}

		if (originalName != name) {
			spdlog::warn("Attempt to create an entity with existing name '{}', using '{}' instead", originalName, name);
		}

		entityIndicesMap[name] = index;
		entityNames[index]	   = name;


		return Handle(index);
	}

	static void removeEntity() {
		// TODO: removeEntity
	}


	template <typename ComponentType>
	static inline ComponentType& getComponent(uint32_t index) {
		assert(hasComponent<ComponentType>(index));
		return getComponentArray<ComponentType>()[index];
	}

	template <typename ComponentType>
	static inline ComponentType& getComponent(Handle handle) {
		return getComponent<ComponentType>(handle.getIndex());
	}


	template <typename ComponentType>
	static inline std::vector<ComponentType>& getComponentArray() {
		return std::get<std::vector<ComponentType>>(componentArrays);
	}

	template <uint32_t Index>
	static inline auto& getComponentArray() {
		return std::get<Index>(componentArrays);
	}


	template <typename... RequiredComponentTypes, typename Func>
	static void forEach(Func&& func, uint fragmentIndex = 0, uint fragmentCount = 1) {
		auto ranges = getValidRanges<RequiredComponentTypes...>(fragmentIndex, fragmentCount);

		for (auto range : ranges) {
			for (uint32_t i = range.first; i < range.second; i++) {
				func(getComponent<RequiredComponentTypes>(i)...);
			}
		}
	}

	template <typename... RequiredComponentTypes, typename Func>
	static void forEachIndexed(Func&& func, uint fragmentIndex = 0, uint fragmentCount = 1) {
		auto ranges = getValidRanges<RequiredComponentTypes...>(fragmentIndex, fragmentCount);

		for (auto range : ranges) {
			for (uint32_t i = range.first; i < range.second; i++) {
				func(Handle(i), getComponent<RequiredComponentTypes>(i)...);
			}
		}
	}


	template <typename... RequiredComponentTypes, typename Func>
	static void apply(uint32_t index, Func&& func) {
		func(getComponent<RequiredComponentTypes>(index)...);
	}


protected:
	EntityManagerBase() {};


private:
	static void reserve(uint32_t size) {
		((std::get<std::vector<ComponentTypes>>(componentArrays).resize(size)), ...);
		entityNames.resize(size);
	}


	// Resets all components of an entity to their initial state
	static void resetEntity(uint32_t index) {
		resetEntityImpl(index, std::make_index_sequence<getComponentTypeCount()>());
	}

	template <std::size_t... Indices>
	static void resetEntityImpl(uint32_t index, std::index_sequence<Indices...>) {
		((std::get<Indices>(componentArrays)[index] = {}), ...);
	}


	template <typename... RequiredComponentTypes>
	static Ranges getValidRanges(uint fragmentIndex, uint fragmentCount) {
		Ranges ranges;

		auto leastSignificantTypeIndex = getLeastSignificantTypeIndex<RequiredComponentTypes...>();

		std::array<uint32_t, sizeof...(RequiredComponentTypes)> requiredTypeIndices { (
			getComponentTypeIndex<RequiredComponentTypes>(), ...) };

		std::array<uint32_t, sizeof...(ComponentTypes)> componentTypeIterators { 0 };
		for (uint32_t currentRangeIndex = 0; currentRangeIndex < componentRanges[leastSignificantTypeIndex].size();
			 currentRangeIndex++) {
			std::pair range = componentRanges[leastSignificantTypeIndex][currentRangeIndex];

			for (auto requiredTypeIndex : requiredTypeIndices) {
				auto comparableRange = componentRanges[requiredTypeIndex][componentTypeIterators[requiredTypeIndex]];
				range.first			 = std::max(range.first, comparableRange.first);
				range.second		 = std::min(range.second, comparableRange.second);
			}

			if (range.first < range.second) {
				ranges.push_back(range);
			}


			for (auto requiredTypeIndex : requiredTypeIndices) {
				if (range.second >=
					componentRanges[requiredTypeIndex][componentTypeIterators[requiredTypeIndex]].second) {

					componentTypeIterators[requiredTypeIndex]++;

					// Stop searching if there are no more ranges of required type left
					if (componentTypeIterators[requiredTypeIndex] >=
						componentRanges[leastSignificantTypeIndex].size()) {
						currentRangeIndex = componentRanges[leastSignificantTypeIndex].size();
						break;
					}
				}
			}
		}

		// Make sure first fallback entity won't be used
		if (ranges.size() > 0) {
			ranges[0].first = std::max(1U, ranges[0].first);
		}


		// Shrink ranges to fit fragment

		uint totalEntities = 0;
		for (const auto& range : ranges) {
			totalEntities += range.second - range.first;
		}

		uint entitiesPerFragment = std::ceil(static_cast<float>(totalEntities) / fragmentCount);
		uint leftShiftCount		 = entitiesPerFragment * fragmentIndex;
		uint rightShiftCount	 = totalEntities - std::min(leftShiftCount + entitiesPerFragment, totalEntities);

		for (auto& range : ranges) {
			auto offset = std::min(range.second - range.first, leftShiftCount);
			range.first += offset;
			leftShiftCount -= offset;
		}

		for (int i = ranges.size() - 1; i >= 0; i--) {
			auto& range = ranges[i];

			auto offset = std::min(range.second - range.first, rightShiftCount);
			range.second -= offset;
			rightShiftCount -= offset;
		}

		return ranges;
	}


	// fixex ranges at given position to match signature
	static void fixRangesAtPosition(uint32_t position, uint32_t signature) {
		for (uint32_t componentTypeIndex = 0; componentTypeIndex < getComponentTypeCount(); componentTypeIndex++) {
			if ((signature & (1 << (getComponentTypeCount() - 1 - componentTypeIndex))) != 0) {
				// position should be within range
				for (uint32_t rangeIndex = 0; rangeIndex < componentRanges[componentTypeIndex].size() - 1;
					 rangeIndex++) {
					auto& firstRange  = componentRanges[componentTypeIndex][rangeIndex];
					auto& secondRange = componentRanges[componentTypeIndex][rangeIndex + 1];

					if (firstRange.second <= position && position < secondRange.first) {
						if (firstRange.second == position) {
							firstRange.second++;
						}
						if (secondRange.first == position + 1) {
							secondRange.first--;
						}
						if (firstRange.second >= secondRange.first - 1) {
							// merge ranges if they connect or overlap
							firstRange.second = secondRange.second;
							componentRanges[componentTypeIndex].erase(componentRanges[componentTypeIndex].begin() +
																	  rangeIndex + 1);
						}
					}
				}

				// exceptional case: position is to the right of the last range
				auto& lastRange = componentRanges[componentTypeIndex][componentRanges[componentTypeIndex].size() - 1];
				if (lastRange.second == position) {
					lastRange.second++;
				} else if (lastRange.second < position) {
					std::pair newRange = { position, position + 1 };
					componentRanges[componentTypeIndex].push_back(newRange);
				}
			} else {
				// position should be outside of range
				for (uint32_t rangeIndex = 0; rangeIndex < componentRanges[componentTypeIndex].size(); rangeIndex++) {
					auto& range = componentRanges[componentTypeIndex][rangeIndex];

					if (range.first <= position && position < range.second) {
						if (range.first == position) {
							range.first++;
						} else if (range.second == position - 1) {
							range.second--;
						} else {
							std::pair newRange = { position + 1, range.second };
							range.second	   = position;
							componentRanges[componentTypeIndex].insert(
								componentRanges[componentTypeIndex].begin() + rangeIndex + 1, newRange);
						}
					}
				}
			}
		}
		assert(getSignature(position) == signature);
	}

	// returns left edge ranges which include both inner and outer ranges
	static auto getLeftRangeEdges(uint32_t index) {
		std::array<uint32_t, getComponentTypeCount()> leftRangeEdges;

		for (uint32_t typeIndex = 0; typeIndex < leftRangeEdges.size(); typeIndex++) {
			for (uint32_t rangeIndex = 0; rangeIndex < componentRanges[typeIndex].size(); rangeIndex++) {
				if (componentRanges[typeIndex][rangeIndex].first > index) {
					break;
				}
				leftRangeEdges[typeIndex] = componentRanges[typeIndex][rangeIndex].first;
				if (componentRanges[typeIndex][rangeIndex].second > index) {
					break;
				}
				leftRangeEdges[typeIndex] = componentRanges[typeIndex][rangeIndex].second;
			}
		}

		return leftRangeEdges;
	}

	template <uint32_t ComponentTypeIndex>
	static bool hasComponent(uint32_t index) {
		for (auto& range : componentRanges[ComponentTypeIndex]) {
			if (range.first <= index && index < range.second) {
				return true;
			}
		}
		return false;
	}

	template <typename ComponentType>
	static bool hasComponent(uint32_t index) {
		return hasComponent<getComponentTypeIndex<ComponentType>()>(index);
	}

	template <typename... RequiredComponentTypes>
	static constexpr uint32_t getSignature() {
		// bit order reversed so signature of ComponentType0 is larger than ComponentType1
		return ((1 << (getComponentTypeCount() - 1 - getComponentTypeIndex<RequiredComponentTypes>())) + ...);
	}

	static uint32_t getSignature(uint32_t index) {
		return getSignatureImpl(index, std::make_index_sequence<getComponentTypeCount()>());
	}

	template <std::size_t... Indices>
	static uint32_t getSignatureImpl(uint32_t index, std::index_sequence<Indices...>) {
		return (((1 << (getComponentTypeCount() - 1 - Indices)) * hasComponent<Indices>(index)) | ...);
	}


	// moves component data only and does not change signature!
	static void moveEntity(uint32_t from, uint32_t to) {
		moveEntityImpl(from, to, std::make_index_sequence<getComponentTypeCount()>());

		entityNames[to]					  = entityNames[from];
		entityIndicesMap[entityNames[to]] = to;
	}

	template <std::size_t... Indices>
	static void moveEntityImpl(uint32_t from, uint32_t to, std::index_sequence<Indices...>) {
		((std::get<Indices>(componentArrays)[to] = std::get<Indices>(componentArrays)[from]), ...);
	}


	template <typename... RequiredComponentTypes>
	static uint32_t getLeastSignificantTypeIndex() {
		uint32_t index = 0;
		((index = std::max(index, getComponentTypeIndex<RequiredComponentTypes>())), ...);

		return index;
	}

	static constexpr uint32_t getComponentTypeCount() {
		return sizeof...(ComponentTypes);
	}

	template <typename ComponentType>
	static constexpr uint32_t getComponentTypeIndex() {
		// return std::variant<ComponentTypes...>(ComponentType()).index();
		return getComponentTypeIndexImpl<ComponentType>(std::make_index_sequence<sizeof...(ComponentTypes)>());
	}

	template <typename ComponentType, std::size_t... Indices>
	static constexpr uint32_t getComponentTypeIndexImpl(std::index_sequence<Indices...>) {
		return ((std::is_same<typename std::tuple_element<Indices, std::tuple<ComponentTypes...>>::type,
							  ComponentType>::value
					 ? Indices
					 : 0) +
				...);
	}
};

template <typename... ComponentTypes>
std::tuple<std::vector<ComponentTypes>...> EntityManagerBase<ComponentTypes...>::componentArrays {};

template <typename... ComponentTypes>
std::unordered_map<std::string, uint32_t> EntityManagerBase<ComponentTypes...>::entityIndicesMap {};

template <typename... ComponentTypes>
std::vector<std::string> EntityManagerBase<ComponentTypes...>::entityNames {};

template <typename... ComponentTypes>
std::array<typename EntityManagerBase<ComponentTypes...>::Ranges, sizeof...(ComponentTypes)>
	EntityManagerBase<ComponentTypes...>::componentRanges {};
} // namespace Engine