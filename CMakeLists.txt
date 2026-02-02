cmake_minimum_required(VERSION 4.0)
project(R-ECS
    VERSION 0.2.1
    DESCRIPTION "Implementation of the Replication-Entities-Components-Systems in C++ using templates and modern language features."
    LANGUAGES CXX
)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    SET(TEST_ENABLE ON)
    add_compile_definitions(ECS_TESTING)
else ()
    SET(TEST_ENABLE OFF)
endif ()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(R-ECS
    STATIC
        src/src.cpp
        src/component/RegistryFactory.cpp
)

target_include_directories(R-ECS
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(TEST_ENABLE)
    message("= = = = = = = = = TEST enable = = = = = = = = =")
    include(FetchContent)

    FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG v1.14.0
    )
    FetchContent_MakeAvailable(googletest)

    add_executable(test_recs
            test/ecs/components_test.cpp
            test/ecs/registry_tests.cpp
            test/reg/registrator_factory_tests.cpp
    )

    target_link_libraries(test_recs
        PRIVATE
            GTest::gtest_main
            R-ECS
    )
    include(GoogleTest)
    gtest_discover_tests(test_recs)
else ()
    message("TEST disable")
endif()
