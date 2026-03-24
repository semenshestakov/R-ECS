#include "collections/Context.hpp"
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

// Fixture для Context
class ContextTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ctx = std::make_unique<Context>();
    }

    std::unique_ptr<Context> ctx;
};

TEST_F(ContextTest, EmplaceAndGet)
{
    auto& t = ctx->emplace<TestStruct>();
    EXPECT_EQ(t.value, 42);

    auto& t2 = ctx->get<TestStruct>();
    EXPECT_EQ(&t, &t2); // должен быть тот же объект
}

TEST_F(ContextTest, HasMethod)
{
    EXPECT_FALSE(ctx->has<TestStruct>());
    ctx->emplace<TestStruct>();
    EXPECT_TRUE(ctx->has<TestStruct>());
}

TEST_F(ContextTest, RemoveMethod)
{
    ctx->emplace<TestStruct>();
    EXPECT_TRUE(ctx->has<TestStruct>());

    ctx->remove<TestStruct>();
    EXPECT_FALSE(ctx->has<TestStruct>());
}


TEST_F(ContextTest, GetOrEmplaceReturnsExisting)
{
    auto& t1 = ctx->emplace<TestStruct>();
    t1.value = 123;

    auto& t2 = ctx->getOrEmplace<TestStruct>();
    EXPECT_EQ(t2.value, 123);
}


TEST_F(ContextTest, GetOrEmplaceCreatesNewIfMissing)
{
    EXPECT_FALSE(ctx->has<AnotherStruct>());

    auto& a = ctx->getOrEmplace<AnotherStruct>("Hello");
    EXPECT_EQ(a.name, "Hello");
    EXPECT_TRUE(ctx->has<AnotherStruct>());
}


TEST_F(ContextTest, MultipleTypesStoredSeparately)
{
    auto& t = ctx->emplace<TestStruct>();
    auto& a = ctx->emplace<AnotherStruct>("World");

    t.value = 555;
    a.name = "Test";

    auto& t2 = ctx->get<TestStruct>();
    auto& a2 = ctx->get<AnotherStruct>();

    EXPECT_EQ(t2.value, 555);
    EXPECT_EQ(a2.name, "Test");
}
