#include "ecs/Registry.hpp"

#include <utility>
#include "ecs/utils/RegistryError.hpp"


ecs::Registry::Registry(SystemsManager systemManager) :
    m_systemManager(std::move(systemManager))
{
}

bool ecs::Registry::Init(void* args /* = nullptr */)
{
    return m_systemManager.Init({"", args, m_eventSystem});
}

void ecs::Registry::Update()
{
    m_systemManager.Update(*this);
}
