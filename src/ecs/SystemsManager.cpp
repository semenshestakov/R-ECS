#include <ranges>
#include "event/EventUtils.hpp"
#include "ecs/systems/SystemsManager.hpp"

#include "ecs/Registry.hpp"
#include "ecs/jobs/ThreadAffinity.hpp"
#include "ecs/systems/SystemRegistrator.hpp"


ecs::SystemsManager::SystemsManager() = default;

ecs::SystemsManager::~SystemsManager() = default;

ecs::SystemsManager::SystemsManager(const SystemsManager& other) { this->copy(other); }

ecs::SystemsManager& ecs::SystemsManager::operator=(const SystemsManager& other)
{
    this->copy(other);
    return *this;
}

void ecs::SystemsManager::copy(const SystemsManager& other)
{
    m_schedule.clear();
    m_systemsMap.clear();

    m_schedule = other.m_schedule;
    m_disabled = other.m_disabled;
    m_subscriptionsDirty = true;
    for(auto const& [hash, systemPtr]: other.m_systemsMap)
    {
        m_systemsMap[hash] = baseSystemPtr_t(systemPtr->New());
    }
}

ecs::SystemsManager::SystemsManager(SystemsManager&& other) noexcept { this->swap(other); }

ecs::SystemsManager& ecs::SystemsManager::operator=(SystemsManager&& other) noexcept
{
    this->swap(other);
    return *this;
}

void ecs::SystemsManager::swap(SystemsManager& other) noexcept
{
    std::swap(m_systemsMap, other.m_systemsMap);
    std::swap(m_schedule, other.m_schedule);
    std::swap(m_disabled, other.m_disabled);
    std::swap(m_subscriptionsDirty, other.m_subscriptionsDirty);
}

std::unordered_set<ecs::systemHash_t> ecs::SystemsManager::effectiveDisabled() const
{
    if(m_disabled.empty())
        return {};

    std::unordered_set<systemHash_t> disabled = m_schedule.CollectHardDependents(m_disabled);
    disabled.insert(m_disabled.begin(), m_disabled.end());
    return disabled;
}

void ecs::SystemsManager::Disable(const systemHash_t hash)
{
    ECS_ASSERT_MAIN_THREAD("SystemsManager::Disable");
    if(m_disabled.insert(hash).second)
        m_subscriptionsDirty = true;
}

void ecs::SystemsManager::Enable(const systemHash_t hash)
{
    ECS_ASSERT_MAIN_THREAD("SystemsManager::Enable");
    if(m_disabled.erase(hash) != 0)
        m_subscriptionsDirty = true;
}

bool ecs::SystemsManager::IsEnabled(const systemHash_t hash) const
{
    return !effectiveDisabled().contains(hash);
}

bool ecs::SystemsManager::isScheduleDirty() const
{
    return m_schedule.isDirty();
}

bool ecs::SystemsManager::needsResubscribe() const
{
    return m_subscriptionsDirty;
}

bool ecs::SystemsManager::Init(const InitState& state)
{
    for(auto& system: m_systemsMap | std::views::values)
    {
        if(!system->isInit())
            system->Init(state);
    }
    m_schedule.Init();
    return true;
}

bool ecs::SystemsManager::Subscribe(const SubscribeState& state)
{
    m_schedule.Build();
    const std::unordered_set<systemHash_t> disabled = effectiveDisabled();
    event::priority_t priority = event::MAX_PRIORITY;
    for(const auto& stageSystems: m_schedule)
    {
        for(auto& systemHash: stageSystems)
        {
            if(disabled.contains(systemHash))
            {
                m_systemsMap[systemHash]->Unsubscribe();         // detach handlers of disabled systems (cascade included)
                continue;
            }

            SubscribeState localState = state;
            localState.priority = priority;

            m_systemsMap[systemHash]->Subscribe(localState);     // one-shot: subscribes new systems
        }
        --priority;
    }
    m_subscriptionsDirty = false;
    return true;
}

void ecs::SystemsManager::Update(Registry& registry)
{
    m_schedule.Build();
    const std::unordered_set<systemHash_t> disabled = effectiveDisabled();
    const UpdateState state{};
    IJobScheduler& scheduler = registry.Scheduler();

    for(const auto& stageSystems: m_schedule)
    {
        scheduler.ParallelFor(
            0, stageSystems.size(), 1, [&](const std::size_t first, const std::size_t last)
            {
                for(std::size_t i = first; i < last; ++i)
                {
                    const systemHash_t systemHash = stageSystems[i];
                    if(disabled.contains(systemHash))
                        continue;

                    m_systemsMap.at(systemHash)->Update(registry, state);
                }
            });
    }
}

bool ecs::SystemsManager::Register(const std::size_t systemRegIndex)
{
    ECS_ASSERT_MAIN_THREAD("SystemsManager::Register");

    const auto& systemInfo = SystemRegistrator::Get(systemRegIndex);
    if(m_systemsMap.contains(systemInfo.hash))
        return false;

    m_systemsMap[systemInfo.hash] = systemInfo.makeNew();
    m_schedule.Add(*m_systemsMap[systemInfo.hash].get(), systemInfo.hash);
    return true;
}

std::size_t ecs::SystemsManager::size() const
{
    return m_systemsMap.size();
}

ecs::SystemsManager ecs::SystemsManager::Create(const std::span<const std::size_t> systemRegIndexes)
{
    SystemsManager systemsManager;
    for (const size_t systemIndex: systemRegIndexes)
    {
        systemsManager.Register(systemIndex);
    }

    return systemsManager;
}
