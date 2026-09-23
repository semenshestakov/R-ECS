#pragma once
#include <cassert>
#include "../SingletonStore.hpp"


template <typename Alloc, typename Mutex>
template <typename T, typename... Args>
T& collections::SingletonStore<Alloc, Mutex>::emplace(Args&&... args)
{
    [[maybe_unused]] auto guard = this->lock_guard();
    return emplaceImpl<T>(std::forward<Args>(args)...);
}

template <typename Alloc, typename Mutex>
template <typename T>
T& collections::SingletonStore<Alloc, Mutex>::get()
{
    [[maybe_unused]] auto guard = this->lock_guard();

    auto id = getStoreKey<T>();
    assert(m_data.contains(id) && "Context: type not found");

    auto* holder = static_cast<Holder<T>*>(m_data.at(id).get());
    return holder->value;
}

template <typename Alloc, typename Mutex>
template <typename T>
const T& collections::SingletonStore<Alloc, Mutex>::get() const
{
    [[maybe_unused]] auto guard = this->lock_guard();

    auto id = getStoreKey<T>();
    assert(m_data.contains(id) && "Context: type not found");

    auto* holder = static_cast<Holder<T>*>(m_data.at(id).get());
    return holder->value;
}

template <typename Alloc, typename Mutex>
template <typename T, typename... Args>
T& collections::SingletonStore<Alloc, Mutex>::getOrEmplace(Args&&... args)
{
    [[maybe_unused]] auto guard = this->lock_guard();

    auto id = getStoreKey<T>();
    auto it = m_data.find(id);
    if (it != m_data.end())
        return static_cast<Holder<T>*>(it->second.get())->value;

    return this->template emplaceImpl<T>(std::forward<Args>(args)...);
}

template <typename Alloc, typename Mutex>
template <typename T>
bool collections::SingletonStore<Alloc, Mutex>::has() const
{
    [[maybe_unused]] auto guard = this->lock_guard();
    return m_data.contains(getStoreKey<T>());
}

template <typename Alloc, typename Mutex>
template <typename T>
void collections::SingletonStore<Alloc, Mutex>::remove()
{
    [[maybe_unused]] auto guard = this->lock_guard();
    m_data.erase(getStoreKey<T>());
}

template <typename Alloc, typename Mutex>
template <typename T, typename... Args>
typename collections::SingletonStore<Alloc, Mutex>::HolderPtr collections::SingletonStore<Alloc, Mutex>::makeHolder(Args&&... args)
{
    if constexpr (AllocatorPolicy<Alloc>::value)
    {
        using ReboundAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<Holder<T>>;
        using ReboundTraits = std::allocator_traits<ReboundAlloc>;

        ReboundAlloc alloc(this->getAllocator());
        Holder<T>* raw = ReboundTraits::allocate(alloc, 1);

        try
        {
            ReboundTraits::construct(alloc, raw, std::forward<Args>(args)...);
        }
        catch (...)
        {
            ReboundTraits::deallocate(alloc, raw, 1);
            throw;
        }

        return HolderPtr(raw, [alloc](BaseHolder* base) mutable {
            auto* typed = static_cast<Holder<T>*>(base);
            ReboundTraits::destroy(alloc, typed);
            ReboundTraits::deallocate(alloc, typed, 1);
        });
    }
    else
    {
        return HolderPtr(new Holder<T>(std::forward<Args>(args)...), [](BaseHolder* base) { delete base; });
    }
}

template <typename Alloc, typename Mutex>
template <typename T, typename... Args>
T& collections::SingletonStore<Alloc, Mutex>::emplaceImpl(Args&&... args)
{
    auto id = getStoreKey<T>();

    auto ptr = makeHolder<T>(std::forward<Args>(args)...);
    T& ref = static_cast<Holder<T>*>(ptr.get())->value;

    m_data.insert_or_assign(id, std::move(ptr));
    return ref;
}

template <typename Alloc, typename Mutex>
template <typename T>
constexpr typename collections::SingletonStore<Alloc, Mutex>::ctxId_t collections::SingletonStore<Alloc, Mutex>::getStoreKey()
{
    return typeid(T).hash_code();
}
