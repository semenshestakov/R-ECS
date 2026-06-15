#include <memory>
#include <utility>
#include "ecs/Registry.hpp"
#include "ecs/jobs/SerialJobScheduler.hpp"
#include "ecs/registry/RegistryRegistrator.hpp"


ecs::Registry::Registry(SystemsManager systemManager) :
    m_systemManager(std::move(systemManager)),
    m_eventSystem(*this),
    m_scheduler(std::make_unique<SerialJobScheduler>())
{}

ecs::Registry::Registry() :
     m_eventSystem(*this),
     m_scheduler(std::make_unique<SerialJobScheduler>())
{}

void ecs::Registry::SetScheduler(std::unique_ptr<IJobScheduler> scheduler)
{
    m_scheduler = scheduler ? std::move(scheduler) : std::make_unique<SerialJobScheduler>();
}

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

    if (m_systemManager.isScheduleDirty() || m_systemManager.needsResubscribe())
        m_systemManager.Subscribe({m_eventSystem}, {});

    m_systemManager.Update(*this, {});
    m_eventSystem.FlushEvents({});
    m_commandQueue.Flush({}, *this);
}

ecs::Registry ecs::Registry::Create(const std::string& name)
{
    return Registry{SystemsManager::Create(RegistryRegistrator::Get(name).systemRegIndexes)};
}
