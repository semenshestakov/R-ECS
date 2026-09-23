#pragma once
#include <cstdint>
#include <functional>
#include <limits>


namespace event
{

    // = = = = = = = = = = = = = = = = = = = = = = types = = = = = = = = = = = = = = = = = = = = = =
    using callbackId_t = std::uint32_t;
    using eventId_t = std::uint32_t;
    constexpr eventId_t INVALID_EVENT_ID = 0;
    constexpr eventId_t INVALID_CALLBACK_ID = 0;
    using deleter_t = std::function<void(callbackId_t)>;

    using priority_t = std::int32_t;
    constexpr priority_t DEFAULT_PRIORITY = 0;
    constexpr priority_t MAX_PRIORITY = std::numeric_limits<priority_t>::max();

    template<typename... Args> using eventCallback_t = std::function<void(Args...)>;

    // = = = = = = = = = = = = = = = = = = = = make callback = = = = = = = = = = = = = = = = = = = = 
    #define MAKE_METHOD_CALLBACK(pInstance, method) \
        [pInstance](auto&&... args) { (pInstance)->method(std::forward<decltype(args)>(args)...); }

    /**
     * @brief Creates a callback from a member function pointer.
     *
     * @tparam T Class type of the instance
     * @tparam Args Callback argument types
     * @param pInstance Pointer to the object whose member function will be called
     * @param method Member function pointer to bind
     * @return eventCallback_t<Args...> Callable that invokes method on pInstance
     *
     * @note The caller must ensure pInstance outlives the returned callback.
     */
    template<typename T, typename... Args>
    eventCallback_t<Args...> makeMethodCallback(T* pInstance, void (T::*method)(Args...))
    {
        return [pInstance, method](Args... args) { (pInstance->*method)(std::forward<Args>(args)...); };
    }

    // = = = = = = = = = = = = = = = = = = = = = concepts = = = = = = = = = = = = = = = = = = = = = =
    template<typename T> concept ValidCallbackIdsContainer = requires(T container) 
    {
        { *container.begin() } -> std::convertible_to<callbackId_t>;
        { *container.end() }   -> std::convertible_to<callbackId_t>;
    };

    template<typename T> concept ValidCallbackIdType = std::is_same_v<T, callbackId_t> || ValidCallbackIdsContainer<T>;

} // end namespace Event 