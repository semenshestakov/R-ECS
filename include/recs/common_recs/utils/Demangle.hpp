#pragma once
#include <string>

#if defined(__has_include)
#if __has_include(<cxxabi.h>)
#define RECS_HAS_CXXABI 1
#include <cxxabi.h>
#include <cstdlib>
#include <memory>
#endif
#endif

#ifndef RECS_HAS_CXXABI
#define RECS_HAS_CXXABI 0
#endif


namespace ecs
{

    /**
     * @brief Converts a mangled type name (from typeid(T).name()) into a human-readable one.
     *
     * Uses the Itanium ABI demangler when <cxxabi.h> is available (GCC/Clang with
     * libstdc++ or libc++). On toolchains without it (e.g. MSVC, or Clang targeting
     * the MSVC ABI, where typeid().name() is already readable) it returns the input
     * unchanged.
     */
    inline std::string demangle(const char* mangled)
    {
        if (mangled == nullptr)
            return {};

#if RECS_HAS_CXXABI
        int status = 0;
        const std::unique_ptr<char, void (*)(void*)> demangled(
            abi::__cxa_demangle(mangled, nullptr, nullptr, &status), std::free);

        return (status == 0 && demangled) ? std::string(demangled.get()) : std::string(mangled);
#else
        return std::string(mangled);
#endif
    }

}
