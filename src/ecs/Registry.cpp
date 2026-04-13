#include "ecs/Registry.hpp"

#include <utility>
#include "ecs/utils/RegistryError.hpp"


ecs::Registry::Registry(SystemsManager systemManager) :
    m_systemManager(std::move(systemManager))
{
}

bool ecs::Registry::Init(void* args /* = nullptr */)
{
    const bool result = m_systemManager.Init({"", args});
    if (result)
        return result&m_systemManager.Subscribe({m_eventSystem});
    return result;
}

void ecs::Registry::Update()
{
    m_systemManager.Update(*this);
}
