#include "ecs/Registry.hpp"

#include <utility>
#include "ecs/utils/RegistryError.hpp"


namespace ecs
{

    Registry::Registry(SystemsManager systemManager) : m_systemManager(std::move(systemManager))
    {
    }

    bool Registry::Init(void* args /* = nullptr */)
    {
        return m_systemManager.Init({"", args, m_eventSystem});
    }

    void Registry::Update()
    {
        m_systemManager.Update(*this);
    }

} // namespace ecs
