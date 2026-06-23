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
        src/recs/Registry.cpp
        src/recs/EntityWrapper.cpp
        src/recs/SystemsManager.cpp
        src/recs/SystemsSchedule.cpp
        src/recs/Commands.cpp
        src/recs/ThreadAffinity.cpp
)

target_include_directories(R-ECS
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include/recs
)

# = = = = = = = = = = = = = = = = = = = = = = = = = all CMake options = = = = = = = = = = = = = = = = = = = = = = = = =

option(IS_DOCS_GEN "Configure for documentation generation only (skip tests/benchmarks)" OFF)
option(EXAMPLES_ENABLE "Build R-ECS examples (skip tests/benchmarks)" OFF)
option(TEST_ENABLE "Configure for tests" OFF)
option(BENCHMARK_ENABLE "Configure for benchmarks" OFF)
option(RECS_BACKEND_ONETBB "Build the R-ECS::back_oneTBB job/service backend (requires oneTBB)" OFF)
option(RECS_ONETBB_FETCH "Fetch oneTBB with FetchContent when find_package(TBB) fails" ON)

# = = = = = = = = = = = = = = = = = = = = = = = = = Optional backends = = = = = = = = = = = = = = = = = = = = = = = = =

if(RECS_BACKEND_ONETBB)
    find_package(TBB CONFIG QUIET)

    if(NOT TBB_FOUND AND RECS_ONETBB_FETCH)
        message("= = = = = = = = = R-ECS fetching oneTBB = = = = = = = = =")
        include(FetchContent)
        set(TBB_TEST OFF CACHE BOOL "" FORCE)           # don't build oneTBB's own tests
        set(TBB_STRICT OFF CACHE BOOL "" FORCE)         # don't let oneTBB warnings fail our build
        set(TBBMALLOC_BUILD OFF CACHE BOOL "" FORCE)    # only the scheduler is needed
        FetchContent_Declare(
                onetbb
                GIT_REPOSITORY https://github.com/uxlfoundation/oneTBB.git
                GIT_TAG v2022.3.0
        )
        FetchContent_MakeAvailable(onetbb)
        set(TBB_FOUND TRUE)
    endif()

    if(TBB_FOUND)
        message("= = = = = = = = = R-ECS oneTBB backend enable = = = = = = = = =")
        add_library(R-ECS-back-onetbb
            STATIC
                src/recs-tbb/TbbJobScheduler.cpp
        )
        add_library(R-ECS::back_oneTBB ALIAS R-ECS-back-onetbb)
        target_link_libraries(R-ECS-back-onetbb
            PUBLIC
                R-ECS
            PRIVATE
                TBB::tbb
        )
        target_include_directories(R-ECS-back-onetbb
            PUBLIC
                ${CMAKE_CURRENT_SOURCE_DIR}/include/recs-tbb
        )
    else()
        message(WARNING "[R-ECS] RECS_BACKEND_ONETBB=ON but oneTBB was not found and RECS_ONETBB_FETCH=OFF — backend skipped")
    endif()
endif()


# = = = = = = = = = = = = = = = = = = = = = = = = = Tests && Benchmark && Examples = = = = = = = = = = = = = = = = = = = = = = = = =

if (EXAMPLES_ENABLE)

elseif (TEST_ENABLE OR BENCHMARK_ENABLE)
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
            test/main.cpp
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
            test/ecs/SystemsAccessAndDisable.cpp
            test/ecs/SystemsResubscribe.cpp
            test/ecs/SystemsToggleEvents.cpp
    )

    target_include_directories(test_recs
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/test/include
    )
    target_link_libraries(test_recs
        PRIVATE
            gtest
            gmock
            R-ECS
    )
    # oneTBB backend tests are compiled only when that backend is built.
    if(TARGET R-ECS-back-onetbb)
        message("= = = = = = = = = R-ECS oneTBB backend tests enable = = = = = = = = =")
        target_sources(test_recs PRIVATE test/jobs/TbbJobScheduler.cpp)
        target_link_libraries(test_recs PRIVATE R-ECS::back_oneTBB)
    endif()
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
        target_compile_options(test_recs PRIVATE -fbracket-depth=1024)
    endif()
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(test_recs PRIVATE -fno-var-tracking-assignments)
    endif()
    # MinGW/PE-COFF: stress tests generate >65535 sections due to 1000 template
    if(MINGW)
        target_compile_options(test_recs PRIVATE -Wa,-mbig-obj)
    endif()
    if(MSVC)
        target_compile_options(test_recs PRIVATE /bigobj)
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
            benchmarks/main.cpp
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


if(EXAMPLES_ENABLE)
    message("= = = = = = = = = R-ECS EXAMPLES enable = = = = = = = = =")

    # Core examples run on the default SerialJobScheduler — link only R-ECS.
    set(RECS_EXAMPLES
        Commands
        Events
        RecipesAndCooking
        Simple
        SystemsWithDependencies
    )

    foreach(EXAMPLE ${RECS_EXAMPLES})
        add_executable(example_${EXAMPLE} examples/${EXAMPLE}.cpp)
        target_link_libraries(example_${EXAMPLE} PRIVATE R-ECS)
    endforeach()

    # Jobs/Services install TbbJobScheduler — they need the oneTBB backend.
    set(RECS_TBB_EXAMPLES
        Jobs
        Services
    )

    if(TARGET R-ECS::back_oneTBB)
        foreach(EXAMPLE ${RECS_TBB_EXAMPLES})
            add_executable(example_${EXAMPLE} examples/${EXAMPLE}.cpp)
            target_link_libraries(example_${EXAMPLE} PRIVATE R-ECS R-ECS::back_oneTBB)
        endforeach()
    else()
        message("[R-ECS] Examples ${RECS_TBB_EXAMPLES} need -DRECS_BACKEND_ONETBB=ON — skipped")
    endif()
else()
    message("[R-ECS] EXAMPLES disable")
endif()
