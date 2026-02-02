#include "reg/Registrator.hpp"
#include <gtest/gtest.h>


using namespace reg;

TEST(RegistratorTest, DefaultRegister)
{
    using TestDefaultRegister = Registrator<NameRegistration, RegistrationStrategy::DEFAULT>;

    {
        auto A1 = TestDefaultRegister::Create<int>("A1");
        auto A2 = TestDefaultRegister::Create<int>("A2");

        auto A1_1 = TestDefaultRegister::Create<int>("A1");
        auto A2_1 = TestDefaultRegister::Create<int>("A2");

        std::size_t i = 0;
        std::vector<std::string> allNames = {"A1", "A2", "A1","A2"};
        for (auto elem : TestDefaultRegister::iter())
        {
            EXPECT_EQ(allNames[i], elem.name);
            ++i;
        }

        EXPECT_EQ(TestDefaultRegister::get("A33"), nullptr);
        EXPECT_EQ(TestDefaultRegister::get("A1")->name, "A1");
    }
    EXPECT_EQ(0, TestDefaultRegister::size());
}

TEST(RegistratorTest, DefaultRegisterBasic)
{
    using TestDefaultRegister = Registrator<NameRegistration, RegistrationStrategy::DEFAULT>;

    // Test 1: Basic registration and iteration
    {
        auto A1 = TestDefaultRegister::Create<int>("A1");
        auto A2 = TestDefaultRegister::Create<int>("A2");
        auto A3 = TestDefaultRegister::Create<int>("A3");

        EXPECT_EQ(TestDefaultRegister::size(), 3);

        // Test iteration order (should be registration order)
        std::vector<std::string> expectedNames = {"A1", "A2", "A3"};
        std::vector<std::string> actualNames;

        for (auto elem : TestDefaultRegister::iter())
        {
            actualNames.push_back(elem.name);
        }

        EXPECT_EQ(actualNames, expectedNames);
    }
    EXPECT_EQ(TestDefaultRegister::size(), 0); // All destroyed
}

TEST(RegistratorTest, DefaultRegisterDuplicateNames)
{
    using TestDefaultRegister = Registrator<NameRegistration, RegistrationStrategy::DEFAULT>;

    // Test 2: Duplicate names allowed in DEFAULT strategy
    {
        auto A1 = TestDefaultRegister::Create<int>("A1");
        auto A1_dup = TestDefaultRegister::Create<double>("A1"); // Same name, different type
        auto A2 = TestDefaultRegister::Create<int>("A2");
        auto A1_dup2 = TestDefaultRegister::Create<float>("A1"); // Third with same name

        EXPECT_EQ(TestDefaultRegister::size(), 4); // All 4 registered separately

        std::vector<std::string> expectedNames = {"A1", "A1", "A2", "A1"};
        std::vector<std::string> actualNames;

        for (auto elem : TestDefaultRegister::iter())
        {
            actualNames.push_back(elem.name);
        }

        EXPECT_EQ(actualNames, expectedNames);
    }
    EXPECT_EQ(TestDefaultRegister::size(), 0);
}

TEST(RegistratorTest, DefaultRegisterGetFunction)
{
    using TestDefaultRegister = Registrator<NameRegistration, RegistrationStrategy::DEFAULT>;

    // Test 3: get() function behavior
    {
        auto A1 = TestDefaultRegister::Create<int>("A1");
        auto A2 = TestDefaultRegister::Create<int>("A2");
        auto A1_dup = TestDefaultRegister::Create<double>("A1"); // Duplicate name

        // get() should return the first match (implementation dependent)
        auto* foundA1 = TestDefaultRegister::get("A1");
        EXPECT_NE(foundA1, nullptr);
        EXPECT_EQ(foundA1->name, "A1");

        auto* foundA2 = TestDefaultRegister::get("A2");
        EXPECT_NE(foundA2, nullptr);
        EXPECT_EQ(foundA2->name, "A2");

        auto* notFound = TestDefaultRegister::get("NonExistent");
        EXPECT_EQ(notFound, nullptr);

        // Verify that get() returns a valid pointer we can use
        if (foundA1)
        {
            // Should be able to access the name
            EXPECT_FALSE(foundA1->name.empty());
        }
    }
    EXPECT_EQ(TestDefaultRegister::size(), 0);
}

TEST(RegistratorTest, DefaultRegisterDestructionOrder)
{
    using TestDefaultRegister = Registrator<NameRegistration, RegistrationStrategy::DEFAULT>;

    // Test 4: Destruction order and size tracking
    {
        EXPECT_EQ(TestDefaultRegister::size(), 0);

        auto A1 = TestDefaultRegister::Create<int>("A1");
        EXPECT_EQ(TestDefaultRegister::size(), 1);

        {
            auto A2 = TestDefaultRegister::Create<int>("A2");
            EXPECT_EQ(TestDefaultRegister::size(), 2);

            {
                auto A3 = TestDefaultRegister::Create<int>("A3");
                EXPECT_EQ(TestDefaultRegister::size(), 3);
            } // A3 destroyed

            EXPECT_EQ(TestDefaultRegister::size(), 2); // Only A1 and A2 remain

            // A2 still accessible
            EXPECT_NE(TestDefaultRegister::get("A2"), nullptr);
        } // A2 destroyed

        EXPECT_EQ(TestDefaultRegister::size(), 1); // Only A1 remains
        EXPECT_NE(TestDefaultRegister::get("A1"), nullptr);
        EXPECT_EQ(TestDefaultRegister::get("A2"), nullptr); // A2 no longer exists
        EXPECT_EQ(TestDefaultRegister::get("A3"), nullptr); // A3 no longer exists
    }
    EXPECT_EQ(TestDefaultRegister::size(), 0);
}


TEST(RegistratorTest, DefaultRegisterMultipleTypes)
{
    using TestDefaultRegister = Registrator<NameRegistration, RegistrationStrategy::DEFAULT>;

    // Test 5: Registration with different types
    {
        // Register different types with same factory
        auto intReg = TestDefaultRegister::Create<int>("Integer");
        auto doubleReg = TestDefaultRegister::Create<double>("Double");
        auto stringReg = TestDefaultRegister::Create<std::string>("String");
        auto vectorReg = TestDefaultRegister::Create<std::vector<int>>("Vector");

        EXPECT_EQ(TestDefaultRegister::size(), 4);

        // All should be accessible regardless of type
        std::vector<std::string> expectedNames = {"Integer", "Double", "String", "Vector"};
        std::vector<std::string> actualNames;

        for (auto elem : TestDefaultRegister::iter())
        {
            actualNames.push_back(elem.name);
            EXPECT_FALSE(elem.name.empty()); // All should have non-empty names
        }

        EXPECT_EQ(actualNames, expectedNames);

        // Verify each can be retrieved
        for (const auto& name : expectedNames)
        {
            auto* found = TestDefaultRegister::get(name);
            EXPECT_NE(found, nullptr);
            EXPECT_EQ(found->name, name);
        }
    }
    EXPECT_EQ(TestDefaultRegister::size(), 0);
}

TEST(RegistratorTest, DefaultRegisterEmptyAndLongNames)
{
    using TestDefaultRegister = Registrator<NameRegistration, RegistrationStrategy::DEFAULT>;

    // Test 6: Edge cases with names
    {
        // Empty name
        auto empty1 = TestDefaultRegister::Create<int>("");
        auto empty2 = TestDefaultRegister::Create<double>(""); // Duplicate empty name

        EXPECT_EQ(TestDefaultRegister::size(), 2);

        auto* emptyFound = TestDefaultRegister::get("");
        EXPECT_NE(emptyFound, nullptr);
        EXPECT_EQ(emptyFound->name, "");

        // Long name
        std::string longName(500, 'X');
        auto longReg = TestDefaultRegister::Create<int>(longName);

        EXPECT_EQ(TestDefaultRegister::size(), 3);

        auto* longFound = TestDefaultRegister::get(longName);
        EXPECT_NE(longFound, nullptr);
        EXPECT_EQ(longFound->name, longName);

        // Special characters
        auto specialReg = TestDefaultRegister::Create<int>("Name.With.Dots");
        auto specialReg2 = TestDefaultRegister::Create<double>("Name-With-Dashes");
        auto specialReg3 = TestDefaultRegister::Create<float>("Name_With_Underscores");

        EXPECT_EQ(TestDefaultRegister::size(), 6);

        // Verify special names can be retrieved
        EXPECT_NE(TestDefaultRegister::get("Name.With.Dots"), nullptr);
        EXPECT_NE(TestDefaultRegister::get("Name-With-Dashes"), nullptr);
        EXPECT_NE(TestDefaultRegister::get("Name_With_Underscores"), nullptr);
    }
    EXPECT_EQ(TestDefaultRegister::size(), 0);
}

TEST(RegistratorTest, DefaultRegisterStressTest)
{
    using TestDefaultRegister = Registrator<NameRegistration, RegistrationStrategy::DEFAULT>;

    // Test 7: Stress test with many registrations
    {
        constexpr int NUM_REGISTRATIONS = 100;
        std::vector<decltype(TestDefaultRegister::Create<int>(""))> registrators;

        // Create many registrations
        for (int i = 0; i < NUM_REGISTRATIONS; ++i)
        {
            std::string name = "Element" + std::to_string(i);
            registrators.emplace_back(TestDefaultRegister::Create<int>(name));
            EXPECT_EQ(TestDefaultRegister::size(), i + 1);
        }

        EXPECT_EQ(TestDefaultRegister::size(), NUM_REGISTRATIONS);

        // Verify all can be retrieved
        for (int i = 0; i < NUM_REGISTRATIONS; ++i)
        {
            std::string name = "Element" + std::to_string(i);
            auto* found = TestDefaultRegister::get(name);
            EXPECT_NE(found, nullptr);
            EXPECT_EQ(found->name, name);
        }

        // Destroy in random order
        while (!registrators.empty())
        {
            registrators.pop_back();
            EXPECT_EQ(TestDefaultRegister::size(), registrators.size());
        }

        EXPECT_EQ(TestDefaultRegister::size(), 0);
    }
}

TEST(RegistratorTest, DefaultRegisterExceptionSafety)
{
    using TestDefaultRegister = Registrator<NameRegistration, RegistrationStrategy::DEFAULT>;

    // Test 8: Exception safety
    {
        try
        {
            auto r1 = TestDefaultRegister::Create<int>("BeforeException");
            auto r2 = TestDefaultRegister::Create<int>("BeforeException2");

            EXPECT_EQ(TestDefaultRegister::size(), 2);

            // Simulate an exception
            throw std::runtime_error("Test exception");

            // This should not execute
            auto r3 = TestDefaultRegister::Create<int>("AfterException");
        }
        catch (const std::runtime_error&)
        {
            // Stack unwinding should have destroyed all automatic objects
            // All registrators in the try block should be destroyed
            EXPECT_EQ(TestDefaultRegister::size(), 0);
        }

        // Verify clean state after exception
        EXPECT_EQ(TestDefaultRegister::size(), 0);
        EXPECT_EQ(TestDefaultRegister::get("BeforeException"), nullptr);
        EXPECT_EQ(TestDefaultRegister::get("BeforeException2"), nullptr);
    }
}

TEST(RegistratorTest, DefaultRegisterConcurrentUsePattern)
{
    using TestDefaultRegister = Registrator<NameRegistration, RegistrationStrategy::DEFAULT>;

    // Test 9: Pattern simulating typical usage
    {
        // Phase 1: Registration phase
        auto logger = TestDefaultRegister::Create<int>("Logger");
        auto config = TestDefaultRegister::Create<int>("Config");
        auto database = TestDefaultRegister::Create<int>("Database");
        auto cache = TestDefaultRegister::Create<int>("Cache");

        EXPECT_EQ(TestDefaultRegister::size(), 4);

        // Phase 2: Usage phase - retrieve and use
        auto* loggerPtr = TestDefaultRegister::get("Logger");
        auto* configPtr = TestDefaultRegister::get("Config");
        auto* databasePtr = TestDefaultRegister::get("Database");
        auto* cachePtr = TestDefaultRegister::get("Cache");

        EXPECT_NE(loggerPtr, nullptr);
        EXPECT_NE(configPtr, nullptr);
        EXPECT_NE(databasePtr, nullptr);
        EXPECT_NE(cachePtr, nullptr);

        // Verify all names
        if (loggerPtr) EXPECT_EQ(loggerPtr->name, "Logger");
        if (configPtr) EXPECT_EQ(configPtr->name, "Config");
        if (databasePtr) EXPECT_EQ(databasePtr->name, "Database");
        if (cachePtr) EXPECT_EQ(cachePtr->name, "Cache");

        // Phase 3: Dynamic addition
        auto metrics = TestDefaultRegister::Create<int>("Metrics");
        EXPECT_EQ(TestDefaultRegister::size(), 5);

        // Phase 4: Cleanup happens automatically when objects go out of scope
    }
    EXPECT_EQ(TestDefaultRegister::size(), 0);
}


/// Test 9: for test RegistratorTest::DefaultRegisterRegisterSize
struct TestNameRegistration
{
    const std::string name;

    template <typename T>
    static TestNameRegistration create(const std::string& name) { return {name}; }
};

using g_TestDefaultRegister = Registrator<TestNameRegistration, RegistrationStrategy::DEFAULT>;
auto g_A0 = g_TestDefaultRegister::Create<int>("A0");


TEST(RegistratorTest, DefaultRegisterRegisterSize)
{
    // Test 9
    using TestDefaultRegister = g_TestDefaultRegister;
    {
        auto A1 = TestDefaultRegister::Create<int>("A1");
        EXPECT_EQ(TestDefaultRegister::size(), 2);
    }
    EXPECT_EQ(TestDefaultRegister::size(), 1);
    {
        TestDefaultRegister::Register<float>("A1");
    }
    TestDefaultRegister::Register<float>("A2");

    EXPECT_EQ(TestDefaultRegister::size(), 3);
}


TEST(RegistratorTest, UniqueRegister)
{
    using TestUniqueRegister = Registrator<NameRegistration, RegistrationStrategy::UNIQUE>;

    // Test 1: Basic registration of unique names
    {
        auto A1 = TestUniqueRegister::Create<int>("A1");
        auto A2 = TestUniqueRegister::Create<int>("A2");

        // Re-registration of the same names should increase the counter
        auto A1_copy = TestUniqueRegister::Create<double>("A1");
        auto A2_copy = TestUniqueRegister::Create<float>("A2");

        // Size check - there should be only 2 unique elements
        EXPECT_EQ(TestUniqueRegister::size(), 2);

        // Iteration check - there should be only 2 elements
        std::size_t i = 0;
        std::vector<std::string> expectedNames = {"A1", "A2"};
        for (auto elem : TestUniqueRegister::iter())
        {
            EXPECT_EQ(expectedNames[i], elem.name);
            ++i;
        }
        EXPECT_EQ(i, 2);

        // Checking the receipt of items
        EXPECT_EQ(TestUniqueRegister::get("A1")->name, "A1");
        EXPECT_EQ(TestUniqueRegister::get("A2")->name, "A2");
        EXPECT_EQ(TestUniqueRegister::get("A3"), nullptr);
    }
    // All registrars are destroyed
    EXPECT_EQ(TestUniqueRegister::size(), 0);

    // Test 2: Destroying copies should decrease the counter
    {
        auto A1 = TestUniqueRegister::Create<int>("A1");
        EXPECT_EQ(TestUniqueRegister::size(), 1);

        {
            auto A1_copy = TestUniqueRegister::Create<double>("A1");
            EXPECT_EQ(TestUniqueRegister::size(), 1); // The size does not change
            EXPECT_EQ(TestUniqueRegister::get("A1")->name, "A1");
        } // A1_copy is destroyed, but A1 still exists

        EXPECT_EQ(TestUniqueRegister::size(), 1);

        {
            auto A2 = TestUniqueRegister::Create<int>("A2");
            EXPECT_EQ(TestUniqueRegister::size(), 2);
        }

        EXPECT_EQ(TestUniqueRegister::size(), 1);
        EXPECT_NE(TestUniqueRegister::get("A1"), nullptr);
        EXPECT_EQ(TestUniqueRegister::get("A2"), nullptr);
    }
    EXPECT_EQ(TestUniqueRegister::size(), 0);

    // Test 3: Multiple copies of the same name
    {
        auto A1 = TestUniqueRegister::Create<int>("A1");
        auto A1_copy1 = TestUniqueRegister::Create<double>("A1");
        auto A1_copy2 = TestUniqueRegister::Create<float>("A1");
        auto A1_copy3 = TestUniqueRegister::Create<char>("A1");

        EXPECT_EQ(TestUniqueRegister::size(), 1);

        {
            auto temp = std::move(A1_copy1);
        }
        EXPECT_EQ(TestUniqueRegister::size(), 1);  // 3 more registrars are alive

        {
            auto temp = std::move(A1_copy2);
        }
        EXPECT_EQ(TestUniqueRegister::size(), 1);  // 2 more registrars are alive

        {
            auto temp = std::move(A1_copy3);
        }
        EXPECT_EQ(TestUniqueRegister::size(), 1); // 1 more registrars are alive

        EXPECT_NE(TestUniqueRegister::get("A1"), nullptr);
        EXPECT_EQ(TestUniqueRegister::get("A1")->name, "A1");
    }
    EXPECT_EQ(TestUniqueRegister::size(), 0);

    // Test 4: Mixed Names
    {
        std::vector<std::string> registeredNames;
        auto r1 = TestUniqueRegister::Create<int>("First");
        auto r2 = TestUniqueRegister::Create<double>("Second");
        auto r3 = TestUniqueRegister::Create<float>("Third");

        auto r1_copy = TestUniqueRegister::Create<char>("First");
        auto r2_copy = TestUniqueRegister::Create<short>("Second");

        auto r4 = TestUniqueRegister::Create<long>("Fourth");

        EXPECT_EQ(TestUniqueRegister::size(), 4);

        std::vector<std::string> expected = {"First", "Second", "Third", "Fourth"};
        std::vector<std::string> actual;
        for (auto elem : TestUniqueRegister::iter())
        {
            actual.push_back(elem.name);
        }

        std::sort(expected.begin(), expected.end());
        std::sort(actual.begin(), actual.end());
        EXPECT_EQ(actual, expected);

        for (const auto& name : expected)
        {
            auto ptr = TestUniqueRegister::get(name);
            EXPECT_NE(ptr, nullptr);
            EXPECT_EQ(ptr->name, name);
        }
    }
    EXPECT_EQ(TestUniqueRegister::size(), 0);

    // Test 5: Move semantics
    {
        auto r1 = TestUniqueRegister::Create<int>("MoveTest");
        EXPECT_EQ(TestUniqueRegister::size(), 1);

        {
            auto r1_moved = std::move(r1);
            EXPECT_EQ(TestUniqueRegister::size(), 1);
            EXPECT_NE(TestUniqueRegister::get("MoveTest"), nullptr);
        }

        EXPECT_EQ(TestUniqueRegister::size(), 0);
        EXPECT_EQ(TestUniqueRegister::get("MoveTest"), nullptr);
    }

    // Test 6: Checking for cleanup after exceptions
    {
        try
        {
            auto r1 = TestUniqueRegister::Create<int>("ExceptionTest");
            auto r2 = TestUniqueRegister::Create<double>("ExceptionTest");

            throw std::runtime_error("Test exception");

            auto r3 = TestUniqueRegister::Create<float>("ExceptionTest");
        }
        catch (const std::runtime_error&)
        {
            EXPECT_EQ(TestUniqueRegister::size(), 0);
        }
    }
}