#pragma once
#include <concepts>
#include <functional>
#include "Entity.hpp"
#include "ecs/components/Components.hpp"
#include "ranges/EntitiesIterator.hpp"


namespace ecs
{

    struct IEntitiesManager
    {
        virtual ~IEntitiesManager() = default;
        [[nodiscard]] virtual ranges::EntitiesIterator begin() const = 0;
        [[nodiscard]] ranges::EntitiesIterator end() const { return {};}

    protected:
        static void setValue(ranges::EntitiesIterator& iterator, const entityOpt_t value) { iterator.m_value = value; }
    };

    template<typename T> concept EntitiesManagerConcept = requires(T manager, entityId_t entityId, ComponentsPtr componentsPtr)
    {
        { manager.find(entityId) } -> std::same_as<Components*>;
        { manager.generateId() } -> std::same_as<entityId_t>;
        { manager.emplace(entityId, std::move(componentsPtr)) } -> std::same_as<void>;
        { std::is_base_of_v<T, IEntitiesManager> };
    };


    class EntitiesManager
    {
    public:
        std::function<Components*(entityId_t)> find = nullptr;

    private:
        std::function<void(entityId_t, ComponentsPtr&&)> emplace = nullptr;
        std::function<entityId_t()> generateId = nullptr;
        std::unique_ptr<IEntitiesManager> m_instance = nullptr;

    public:
        [[nodiscard]] ranges::EntitiesIterator begin() const { return m_instance->begin(); }
        [[nodiscard]] ranges::EntitiesIterator end() const { return m_instance->end(); }

        template <EntitiesManagerConcept T>
        static EntitiesManager Create()
        {
            auto managerUniquePtr = std::make_unique<T>();
            T* managerPtr = managerUniquePtr.get();

            EntitiesManager entitiesManager;

            entitiesManager.find = [managerPtr](entityId_t id) -> Components* {return managerPtr->find(id);};
            entitiesManager.emplace = [managerPtr](entityId_t id, ComponentsPtr&& c) {managerPtr->emplace(id, std::move(c));};
            entitiesManager.generateId = [managerPtr]() -> entityId_t {return managerPtr->generateId();};
            entitiesManager.m_instance = std::move(managerUniquePtr);

            return entitiesManager;
        }

        friend class Registry;
    };

}
