#include "EntityManager.hpp"

#include <spdlog/spdlog.h>


namespace Engine {
int EntityManager::init() {
	spdlog::info("Initializing EntityManager...");
	return EntityManagerBase::init();
}
} // namespace Engine