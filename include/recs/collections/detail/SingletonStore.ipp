#pragma once
#include <cassert>
#include "../SingletonStore.hpp"


template <typename T, typename... Args>
T& collections::SingletonStore::emplace(Args&&... args)
{
    auto id = getStoreKey<T>();

    auto ptr = std::make_unique<Holder<T>>(std::forward<Args>(args)...);
    T& ref = static_cast<Holder<T>*>(ptr.get())->value;

    m_data.emplace(id, std::move(ptr));
    return ref;
}

template<typename T>
T& collections::SingletonStore::get()
{
    auto id = getStoreKey<T>();
    assert(m_data.contains(id) && "Context: type not found");

    auto* holder = static_cast<Holder<T>*>(m_data[id].get());
    return holder->value;
}

template<typename T>
const T& collections::SingletonStore::get() const
{
    auto id = getStoreKey<T>();
    assert(m_data.contains(id) && "Context: type not found");

    auto* holder = static_cast<Holder<T>*>(m_data[id].get());
    return holder->value;
}

template<typename T, typename... Args>
T& collections::SingletonStore::getOrEmplace(Args&&... args)
{
    if(!has<T>())
        return emplace<T>(std::forward<Args>(args)...);

    return get<T>();
}

template<typename T>
bool collections::SingletonStore::has() const
{
    return m_data.contains(getStoreKey<T>());
}

template<typename T>
void collections::SingletonStore::remove()
{
    m_data.erase(getStoreKey<T>());
}

template<typename T>
constexpr collections::SingletonStore::ctxId_t collections::SingletonStore::getStoreKey()
{
    return typeid(T).hash_code();
}
