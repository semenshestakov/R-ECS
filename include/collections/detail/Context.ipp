#pragma once
#include <cassert>
#include "../Context.hpp"


template<typename T, typename... Args>
T& collections::Context::emplace(Args&&... args)
{
    auto id = getCtxT<T>();

    auto ptr = std::make_unique<Holder<T>>(std::forward<Args>(args)...);
    T& ref = static_cast<Holder<T>*>(ptr.get())->value;

    m_data.emplace(id, std::move(ptr));
    return ref;
}

template<typename T>
T& collections::Context::get()
{
    auto id = getCtxT<T>();
    assert(m_data.contains(id) && "Context: type not found");

    auto* holder = static_cast<Holder<T>*>(m_data[id].get());
    return holder->value;
}

template<typename T>
const T& collections::Context::get() const
{
    auto id = getCtxT<T>();
    assert(m_data.contains(id) && "Context: type not found");

    auto* holder = static_cast<Holder<T>*>(m_data[id].get());
    return holder->value;
}

template<typename T, typename... Args>
T& collections::Context::getOrEmplace(Args&&... args)
{
    if(!has<T>())
        return emplace<T>(std::forward<Args>(args)...);

    return get<T>();
}

template<typename T>
bool collections::Context::has() const
{
    return m_data.contains(getCtxT<T>());
}

template<typename T>
void collections::Context::remove()
{
    m_data.erase(getCtxT<T>());
}

template<typename T>
constexpr collections::Context::ctxId_t collections::Context::getCtxT()
{
    return typeid(T).hash_code();
}
