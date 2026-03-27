#pragma once
#include "collections/Context.hpp"
#include "common_recs/utils/ClassUtils.hpp"
#include "entities/EntitiesManager.hpp"
#include "event/EventSystem.hpp"
#include "systems/SystemsManager.hpp"


namespace ecs
{

    class Registry final
    {
    public:
        explicit Registry(SystemsManager  systemManager);

        Registry() = default;                                           ///< Default construction
        ~Registry() = default;                                          ///< Default destructor
        Registry(Registry&&) noexcept = default;                        ///< Move constructible
        Registry& operator=(Registry&&) noexcept = default;             ///< Move assignable
        Registry(const Registry&) = delete;                             ///< Non-copyable
        Registry& operator=(const Registry&) = delete;                  ///< Non-copyable

    DEEP_TEST_PRIVATE_ACCESS:
        EntitiesManager m_entitiesManager;          ///< Underlying entity storage
        EventSystem m_eventSystem;                  ///< Local Event System
        SystemsManager m_systemManager;             ///< Underlying entity storage
        collections::Context m_context;             ///< Context (Data storage)

    public:
        [[nodiscard]] EntitiesManager& Entities() { return m_entitiesManager; }
        [[nodiscard]] const EntitiesManager& Entities() const { return m_entitiesManager; }

        [[nodiscard]] EventSystem& Events() { return m_eventSystem; }
        [[nodiscard]] const EventSystem& Events() const { return m_eventSystem; }

        [[nodiscard]] SystemsManager& Systems() { return m_systemManager; }
        [[nodiscard]] const SystemsManager& Systems() const { return m_systemManager; }

        [[nodiscard]] collections::Context& ctx() { return m_context; }
        [[nodiscard]] const collections::Context& ctx() const { return m_context; }

        [[maybe_unused]] bool Init(void* args = nullptr);

        void Update();
    };

} // namespace ecs
