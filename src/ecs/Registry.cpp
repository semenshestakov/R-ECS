#include <utility>
#include "ecs/Registry.hpp"
#include "ecs/registry/RegistryRegistrator.hpp"


ecs::Registry::Registry(SystemsManager systemManager) :
    m_systemManager(std::move(systemManager)),
    m_eventSystem(*this)
{}

ecs::Registry::Registry() :
     m_eventSystem(*this)
{}

bool ecs::Registry::Init(void* args /* = nullptr */)
{
    const bool result = m_systemManager.Init({"", args}, {});
    if (result)
        return result&m_systemManager.Subscribe({m_eventSystem}, {});
    return result;
}

void ecs::Registry::Update()
{
    m_commandQueue.Flush({}, *this);

    m_systemManager.Update(*this, {});
    m_eventSystem.FlushEvents({});
    m_commandQueue.Flush({}, *this);
}

ecs::Registry ecs::Registry::Create(const std::string& name)
{
    return Registry{SystemsManager::Create(RegistryRegistrator::Get(name).systemRegIndexes)};
}
