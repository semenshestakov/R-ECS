#pragma once
#include <string>

#if defined(__GNUG__) || defined(__clang__)
#include <cxxabi.h>
#include <cstdlib>
#include <memory>
#endif


namespace ecs
{

    /**
     * @brief Converts a mangled type name (from typeid(T).name()) into a human-readable one.
     *
     * Uses the Itanium ABI demangler on GCC/Clang. On other toolchains (e.g. MSVC,
     * where typeid().name() is already readable) it returns the input unchanged.
     */
    inline std::string demangle(const char* mangled)
    {
        if (mangled == nullptr)
            return {};

#if defined(__GNUG__) || defined(__clang__)
        int status = 0;
        const std::unique_ptr<char, void (*)(void*)> demangled(
            abi::__cxa_demangle(mangled, nullptr, nullptr, &status), std::free);

        return (status == 0 && demangled) ? std::string(demangled.get()) : std::string(mangled);
#else
        return std::string(mangled);
#endif
    }

}
