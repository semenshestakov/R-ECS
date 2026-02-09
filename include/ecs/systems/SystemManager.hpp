#pragma once
#include <map>
#include <vector>

#include "SystemUtils.hpp"


namespace ecs
{

    class SystemManager final
    {
    public:
        SystemManager();
        ~SystemManager();

    private:
        SystemManager(const SystemManager& other);
        SystemManager& operator=(const SystemManager& other);
        void copy(const SystemManager& other);

    public:
        SystemManager(SystemManager&& other) noexcept;
        SystemManager& operator=(SystemManager&& other) noexcept;
        void swap(SystemManager& other) noexcept;

    private:

        using systemPtr_t = std::unique_ptr<IBaseSystem>;
        using hash_t = std::size_t;

        std::map<updateTag_t, std::vector<hash_t>> m_updates = {};
        std::map<hash_t, systemPtr_t> m_systemsMap = {};

    public:
        template<typename SystemT>
        bool Register();

        void Update(Registry& registry, std::optional<updateTag_t> updateTag = std::nullopt);

        friend class Registry;

    };


    template<typename SystemT>
    bool SystemManager::Register()
    {
        const hash_t hash = typeid(SystemT).hash_code();
        if (m_systemsMap.contains(hash))
            return false;

        m_systemsMap[hash] = std::make_unique<SystemT>();
        m_updates[SystemT::UPDATE_TAG].push_back(hash);
        return true;
    }

}

