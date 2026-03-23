cmake_minimum_required(VERSION 4.0)
project(R-ECS
    VERSION 0.2.1
    DESCRIPTION "Implementation of the Replication-Entities-Components-Systems in C++ using templates and modern language features."
    LANGUAGES CXX
)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    SET(TEST_ENABLE ON)
    add_compile_definitions(DEEP_TEST_ENABLE)
else ()
    SET(TEST_ENABLE OFF)
endif ()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(R-ECS
    STATIC
        src/ecs/Registry.cpp
        src/ecs/SystemsManager.cpp
        src/ecs/EntitiesIterator.cpp
        src/ecs/ComponentsManager.cpp
)

target_include_directories(R-ECS
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(TEST_ENABLE)
    message("= = = = = = = = = TEST enable = = = = = = = = =")
    include(FetchContent)

    option(BUILD_GMOCK "Build GMock" ON)
    FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG v1.14.0
    )
    FetchContent_MakeAvailable(googletest)

    add_executable(test_recs
            test/ecs/AutoRegistration.cpp
            test/ecs/Components.cpp
            test/ecs/Registry.cpp
            test/ecs/Systems.cpp
            test/ecs/SystemEventHandler.cpp
            test/context/Context.cpp
            test/dag/DirectedAcyclicGraph.cpp
            test/reg/registrator_factory_tests.cpp
            test/event/Event.cpp
            test/event/Listener.cpp
            test/event/EventSystem.cpp
    )
    target_include_directories(R-ECS
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/test/include
    )

    target_link_libraries(test_recs
        PRIVATE
            GTest::gtest_main
            GTest::gmock_main
            R-ECS
    )
    include(GoogleTest)
    gtest_discover_tests(test_recs)
else ()
    message("TEST disable")
endif()
