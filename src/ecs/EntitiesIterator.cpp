#include "ecs/entities/ranges/EntitiesIterator.hpp"


namespace ecs::ranges
{
    EntitiesIterator::EntitiesIterator() : EntitiesIterator(entityNull) {}

    EntitiesIterator::EntitiesIterator(const entityOpt_t value) : EntitiesIterator(value, nullptr) {}

    EntitiesIterator::EntitiesIterator(const entityOpt_t value, std::function<void(EntitiesIterator &)>&& funcInc) :
        m_value(value), m_inc(std::move(funcInc))
    {}

    bool EntitiesIterator::operator==(const entityOpt_t entityOpt) const
    {
        return m_value == entityOpt;
    }

    bool EntitiesIterator::operator!=(const entityOpt_t entityOpt) const
    {
        return !operator==(entityOpt);
    }

    bool EntitiesIterator::operator==(const EntitiesIterator& other) const
    {
        return operator==(other.m_value);
    }

    bool EntitiesIterator::operator!=(const EntitiesIterator& other) const
    {
        return !operator==(other);
    }

    EntitiesIterator& EntitiesIterator::operator++()
    {
        if (m_inc != nullptr && *this != entityNull)
            m_inc(*this);
        return *this;
    }

    EntitiesIterator EntitiesIterator::operator++(int)
    {
        EntitiesIterator copyThis = *this;
        if (m_inc != nullptr)
            m_inc(*this);
        return copyThis;
    }


    entityId_t EntitiesIterator::operator*() const
    {
        return m_value.value();
    }

} // namespace ecs
