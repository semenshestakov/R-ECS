#include "collections/SingletonStore.hpp"
#include <gtest/gtest.h>


using namespace collections;

struct TestStruct
{
    int value = 42;
};

struct AnotherStruct
{
    std::string name;
    AnotherStruct(std::string n) : name(std::move(n)) {}
};

// Fixture для SingletonStore
class ContextTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        store = std::make_unique<SingletonStore>();
    }

    std::unique_ptr<SingletonStore> store;
};

TEST_F(ContextTest, EmplaceAndGet)
{
    auto& t = store->emplace<TestStruct>();
    EXPECT_EQ(t.value, 42);

    auto& t2 = store->get<TestStruct>();
    EXPECT_EQ(&t, &t2); // должен быть тот же объект
}

TEST_F(ContextTest, HasMethod)
{
    EXPECT_FALSE(store->has<TestStruct>());
    store->emplace<TestStruct>();
    EXPECT_TRUE(store->has<TestStruct>());
}

TEST_F(ContextTest, RemoveMethod)
{
    store->emplace<TestStruct>();
    EXPECT_TRUE(store->has<TestStruct>());

    store->remove<TestStruct>();
    EXPECT_FALSE(store->has<TestStruct>());
}


TEST_F(ContextTest, GetOrEmplaceReturnsExisting)
{
    auto& t1 = store->emplace<TestStruct>();
    t1.value = 123;

    auto& t2 = store->getOrEmplace<TestStruct>();
    EXPECT_EQ(t2.value, 123);
}


TEST_F(ContextTest, GetOrEmplaceCreatesNewIfMissing)
{
    EXPECT_FALSE(store->has<AnotherStruct>());

    auto& a = store->getOrEmplace<AnotherStruct>("Hello");
    EXPECT_EQ(a.name, "Hello");
    EXPECT_TRUE(store->has<AnotherStruct>());
}


TEST_F(ContextTest, MultipleTypesStoredSeparately)
{
    auto& t = store->emplace<TestStruct>();
    auto& a = store->emplace<AnotherStruct>("World");

    t.value = 555;
    a.name = "Test";

    auto& t2 = store->get<TestStruct>();
    auto& a2 = store->get<AnotherStruct>();

    EXPECT_EQ(t2.value, 555);
    EXPECT_EQ(a2.name, "Test");
}
