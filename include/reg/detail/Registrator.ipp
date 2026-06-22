#pragma once
#include <algorithm>
#include "../Registrator.hpp"


namespace reg
{

    template <typename FactoryCls, RegistrationStrategy Strategy>
    std::vector<typename Registrator<FactoryCls, Strategy>::vectorItem_t>&
    Registrator<FactoryCls, Strategy>::collection() noexcept
    {
        static std::vector<vectorItem_t> instance;
        return instance;
    }

    template <typename FactoryCls, RegistrationStrategy Strategy>
    Registrator<FactoryCls, Strategy>::~Registrator() noexcept
    {
        if (m_index == INVALID_INDEX)
            return;

        if constexpr (Strategy == RegistrationStrategy::DEFAULT)
        {
            collection()[m_index] = std::nullopt;
            --s_size;
        }
        else if constexpr (Strategy == RegistrationStrategy::UNIQUE)
        {
            --collection()[m_index].first;
            if (collection()[m_index].first == 0)
            {
                collection()[m_index].second = std::nullopt;
                --s_size;
            }
        }

        if (s_size == 0)
            collection().clear();
    }

    template <typename FactoryCls, RegistrationStrategy Strategy>
    Registrator<FactoryCls, Strategy>::Registrator(Registrator&& other) noexcept
    {
        m_index = other.m_index;
        other.m_index = INVALID_INDEX;
    }

    template <typename FactoryCls, RegistrationStrategy Strategy>
    Registrator<FactoryCls, Strategy>& Registrator<FactoryCls, Strategy>::operator=(Registrator&& other) noexcept
    {
        m_index = other.m_index;
        other.m_index = INVALID_INDEX;
        return *this;
    }

    template <typename FactoryCls, RegistrationStrategy Strategy>
    /* static */ void Registrator<FactoryCls, Strategy>::setIndex(Registrator& registrator, const std::size_t index)
    {
        registrator.m_index = index;
    }

    template <typename FactoryCls, RegistrationStrategy Strategy>
    std::size_t Registrator<FactoryCls, Strategy>::getIndex(const std::string& name)
    {
        for (std::size_t i = 0; i < collection().size(); ++i)
        {
            if constexpr (Strategy == RegistrationStrategy::DEFAULT)
            {
                if (collection()[i].has_value() && collection()[i]->name == name)
                    return i;
            }
            else if constexpr (Strategy == RegistrationStrategy::UNIQUE)
            {
                if (collection()[i].second.has_value() && collection()[i].second->name == name)
                    return i;
            }
        }
        return INVALID_INDEX;
    }

    template <typename FactoryCls, RegistrationStrategy Strategy>
    std::size_t Registrator<FactoryCls, Strategy>::getIndex() const
    {
        return m_index;
    }


    template <typename FactoryCls, RegistrationStrategy Strategy>
    Registrator<FactoryCls, Strategy>::Registrator(const std::size_t index) :
        m_index(index)
    {
    }

    template <typename FactoryCls, RegistrationStrategy Strategy>
    template <typename T>
    /* static */ Registrator<FactoryCls, Strategy> Registrator<FactoryCls, Strategy>::Create(const std::string& name)
    {
        std::size_t key;
        if constexpr (Strategy == RegistrationStrategy::DEFAULT)
        {
            key = collection().size();
            collection().emplace_back(FactoryCls::template Create<T>(name));
            ++s_size;
        }
        else if constexpr (Strategy == RegistrationStrategy::UNIQUE)
        {
            auto it = std::find_if(collection().begin(), collection().end(), [&name](const auto& entry) {
                if (entry.second.has_value())
                    return entry.second->name == name;
                return false;
            });

            if (it != collection().end())
            {
                key = std::distance(collection().begin(), it);
                ++(it->first);
            }
            else
            {
                key = collection().size();
                ++s_size;
                collection().emplace_back(std::pair{1u, FactoryCls::template Create<T>(name)});
            }
        }

        return {key};
    }

    template <typename FactoryCls, RegistrationStrategy Strategy>
    template <typename T>
    void Registrator<FactoryCls, Strategy>::Register(const std::string& name)
    {
        auto registrator = Create<T>(name);
        registrator.m_index = INVALID_INDEX;
    }

    template <typename FactoryCls, RegistrationStrategy Strategy>
    std::size_t Registrator<FactoryCls, Strategy>::size()
    {
        return s_size;
    }

    template <typename FactoryCls, RegistrationStrategy Strategy>
    /* static */ FactoryCls* Registrator<FactoryCls, Strategy>::get(const std::string& name)
    {
        return get(getIndex(name));
    }

    template <typename FactoryCls, RegistrationStrategy Strategy>
    FactoryCls* Registrator<FactoryCls, Strategy>::get(const std::size_t key)
    {
        if (key == INVALID_INDEX)
            return nullptr;

        if constexpr (Strategy == RegistrationStrategy::DEFAULT)
        {
            return &collection()[key].value();
        }
        else if constexpr (Strategy == RegistrationStrategy::UNIQUE)
        {
            return &collection()[key].second.value();
        }
        return nullptr;
    }

    template <typename FactoryCls, RegistrationStrategy Strategy>
    bool Registrator<FactoryCls, Strategy>::contains(const std::string& name)
    {
        return get(name) != nullptr;
    }

} // namespace reg
