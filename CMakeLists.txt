cmake_minimum_required(VERSION 4.0)
project(R-ECS
    VERSION 0.11.3
    DESCRIPTION "Implementation of the Replication-Entities-Components-Systems in C++ using templates and modern language features."
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
endif()

add_library(R-ECS
    STATIC
        src/ecs/Registry.cpp
        src/ecs/PrefabEntity.cpp
        src/ecs/EntityWrapper.cpp
        src/ecs/SystemsManager.cpp
        src/ecs/SystemsSchedule.cpp
        src/ecs/Commands.cpp
)

target_include_directories(R-ECS
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)


# = = = = = = = = = = = = = = = = = = = = = = = = = Tests && Benchmark = = = = = = = = = = = = = = = = = = = = = = = = =

option(IS_DOCS_GEN "Configure for documentation generation only (skip tests/benchmarks)" OFF)

if (TEST_ENABLE OR BENCHMARK_ENABLE)
    add_compile_definitions(DEEP_TEST_ENABLE)
elseif (NOT IS_DOCS_GEN AND CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    SET(TEST_ENABLE ON)
    SET(BENCHMARK_ENABLE ON)
    add_compile_definitions(DEEP_TEST_ENABLE)
else ()
    SET(TEST_ENABLE OFF)
    SET(BENCHMARK_ENABLE OFF)
endif ()


# Test executable
if(TEST_ENABLE)
    enable_testing()
    message("= = = = = = = = = R-ECS TEST enable = = = = = = = = =")
    include(FetchContent)

    # Google Tests
    option(BUILD_GMOCK "Build GMock" ON)
    FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG v1.17.0
    )
    FetchContent_MakeAvailable(googletest)

    # Target
    add_executable(test_recs
            test/reg/Registrator.cpp
            test/event/Event.cpp
            test/event/Listener.cpp
            test/event/EventSystem.cpp
            test/collections/BitSet.cpp
            test/collections/Context.cpp
            test/collections/CommandQueue.cpp
            test/collections/DirectedAcyclicGraph.cpp
            test/ecs/AutoRegistration.cpp
            test/ecs/EntitiesManager.cpp
            test/ecs/SystemsEvent.cpp
            test/ecs/EntitiesManagerStressTest.cpp
            test/ecs/Schedule.cpp
            test/ecs/EntityCommands.cpp
            test/ecs/SystemsManager.cpp
    )
    target_include_directories(test_recs
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/test/include
    )
    target_link_libraries(test_recs
        PRIVATE
            gtest_main
            gmock_main
            R-ECS
    )
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
        target_compile_options(test_recs PRIVATE -fbracket-depth=1024)
    endif()
    # MinGW/PE-COFF: stress tests generate >65535 sections due to 1000 template
    if(MINGW)
        target_compile_options(test_recs PRIVATE -Wa,-mbig-obj)
    endif()

    include(GoogleTest)
    gtest_discover_tests(test_recs DISCOVERY_MODE PRE_TEST)
else ()
    message("[R-ECS] TEST disable")
endif()


# Benchmark executable
if (BENCHMARK_ENABLE)

    message("= = = = = = = = = R-ECS BENCHMARK enable = = = = = = = = =")
    include(FetchContent)
    set(BUILD_SHARED_LIBS OFF)

    # Google Benchmark
    FetchContent_Declare(
            benchmark
            GIT_REPOSITORY https://github.com/google/benchmark.git
            GIT_TAG v1.9.5
    )
    FetchContent_MakeAvailable(benchmark)

    # Entt
    FetchContent_Declare(
            entt
            GIT_REPOSITORY https://github.com/skypjack/entt.git
            GIT_TAG v3.16.0
    )
    FetchContent_MakeAvailable(entt)

    # Flecs
    FetchContent_Declare(
            flecs
            GIT_REPOSITORY https://github.com/SanderMertens/flecs.git
            GIT_TAG v4.1.5
    )
    FetchContent_MakeAvailable(flecs)

    # Target
    add_executable(recs_bench
            benchmarks/entities/ecs.cpp
            benchmarks/entities/entt.cpp
            benchmarks/entities/flecs.cpp
    )

    target_include_directories(recs_bench
         PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/include
    )

    target_link_libraries(recs_bench
        PRIVATE
            benchmark::benchmark
            benchmark::benchmark_main
            R-ECS
            EnTT::EnTT
            flecs::flecs
    )
    if(TARGET benchmark)
        target_compile_options(benchmark PRIVATE -Wno-format-nonliteral)
    endif()


else ()
    message("[R-ECS] BENCHMARK disable")
endif()
