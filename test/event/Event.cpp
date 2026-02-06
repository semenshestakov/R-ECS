#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "event/Event.hpp"


using namespace event;
using ::testing::_;
using ::testing::Invoke;

/// Mock classes for testing
class MockCallback
{
public:
    MOCK_METHOD(void, call, ());
    MOCK_METHOD(void, callWithInt, (int));
    MOCK_METHOD(void, callWithString, (const std::string&));
    MOCK_METHOD(void, callWithTwoInts, (int, int));
    MOCK_METHOD(void, callWithMixed, (int, const std::string&, double));
};

/// Test fixture for Event tests
class EventTest : public ::testing::Test
{
protected:
    void SetUp() override { /* Reset callback IDs before each test */ }
};


TEST_F(EventTest, DefaultConstructorCreatesEmptyEvent)
{
    Event<> event;
    SUCCEED();
}


TEST_F(EventTest, IsNonCopyable)
{
    EXPECT_FALSE(std::is_copy_constructible_v<Event<>>);
    EXPECT_FALSE(std::is_copy_assignable_v<Event<>>);
}

TEST_F(EventTest, IsNonMovable)
{
    EXPECT_FALSE(std::is_move_constructible_v<Event<>>);
    EXPECT_FALSE(std::is_move_assignable_v<Event<>>);
}


TEST_F(EventTest, AddCallbackReturnsValidId)
{
    Event<> event;
    MockCallback mock;

    auto callback = [&mock]() { mock.call(); };

    const callbackId_t id1 = event.add(callback);
    const callbackId_t id2 = event.add(callback);

    EXPECT_NE(id1, 0);
    EXPECT_NE(id2, 0);
    EXPECT_NE(id1, id2);
}

TEST_F(EventTest, AddMultipleCallbacksReturnsIncreasingIds)
{
    Event<> event;
    MockCallback mock;
    auto callback = [&mock]() { mock.call(); };

    const callbackId_t id1 = event.add(callback);
    const callbackId_t id2 = event.add(callback);
    const callbackId_t id3 = event.add(callback);

    EXPECT_LT(id1, id2);
    EXPECT_LT(id2, id3);
}


TEST_F(EventTest, TriggerWithNoArguments)
{
    Event<> event;
    MockCallback mock;

    EXPECT_CALL(mock, call()).Times(1);

    event.add([&mock]() { mock.call(); });
    event();
}

TEST_F(EventTest, TriggerWithNoArgumentsMultipleCallbacks)
{
    Event<> event;
    MockCallback mock1, mock2, mock3;

    EXPECT_CALL(mock1, call()).Times(1);
    EXPECT_CALL(mock2, call()).Times(1);
    EXPECT_CALL(mock3, call()).Times(1);

    event.add([&mock1]() { mock1.call(); });
    event.add([&mock2]() { mock2.call(); });
    event.add([&mock3]() { mock3.call(); });

    event();
}


TEST_F(EventTest, TriggerWithIntArgument)
{
    Event<int> event;
    MockCallback mock;

    EXPECT_CALL(mock, callWithInt(42)).Times(1);

    event.add([&mock](int value) { mock.callWithInt(value); });
    event(42);
}


TEST_F(EventTest, TriggerWithStringArgument)
{
    Event<std::string> event;
    MockCallback mock;

    const std::string testString = "Hello World";
    EXPECT_CALL(mock, callWithString(testString)).Times(1);

    event.add([&mock](const std::string& str) { mock.callWithString(str); });
    event(testString);
}


TEST_F(EventTest, TriggerWithTwoIntArguments)
{
    Event<int, int> event;
    MockCallback mock;

    EXPECT_CALL(mock, callWithTwoInts(10, 20)).Times(1);

    event.add([&mock](int a, int b) { mock.callWithTwoInts(a, b); });
    event(10, 20);
}


TEST_F(EventTest, TriggerWithMixedArguments)
{
    Event<int, std::string, double> event;
    MockCallback mock;

    EXPECT_CALL(mock, callWithMixed(5, "test", 3.14)).Times(1);

    event.add([&mock](int a, const std::string& b, double c) {
        mock.callWithMixed(a, b, c);
    });
    event(5, "test", 3.14);
}


TEST_F(EventTest, RemoveExistingCallback)
{
    Event<> event;
    MockCallback mock;

    EXPECT_CALL(mock, call()).Times(0);

    callbackId_t id = event.add([&mock]() { mock.call(); });
    event.remove(id);
    event();
}


TEST_F(EventTest, RemoveNonExistentCallback)
{
    Event<> event;
    MockCallback mock;

    EXPECT_CALL(mock, call()).Times(1);

    event.add([&mock]() { mock.call(); });

    // Removing non-existent ID should not affect existing callbacks
    EXPECT_NO_THROW(event.remove(999));
    event();
}


TEST_F(EventTest, RemoveOneCallbackFromMultiple)
{
    Event<> event;
    MockCallback mock1, mock2, mock3;

    EXPECT_CALL(mock1, call()).Times(0);
    EXPECT_CALL(mock2, call()).Times(1);
    EXPECT_CALL(mock3, call()).Times(1);

    const callbackId_t id1 = event.add([&mock1]() { mock1.call(); });
    event.add([&mock2]() { mock2.call(); });
    event.add([&mock3]() { mock3.call(); });

    event.remove(id1);
    event();
}


TEST_F(EventTest, TriggerEmptyEvent)
{
    Event<> event;

    EXPECT_NO_THROW(event());
}


TEST_F(EventTest, AddAndRemoveMultipleTimes)
{
    Event<int> event;
    MockCallback mock;

    EXPECT_CALL(mock, callWithInt(_)).Times(2);

    auto callback = [&mock](int value) { mock.callWithInt(value); };

    const callbackId_t id1 = event.add(callback);
    event(1);  // Should be called

    event.remove(id1);
    event(2);  // Should not be called

    const callbackId_t id2 = event.add(callback);
    event(3);  // Should be called

    event.remove(id2);
    event(4);  // Should not be called
}


TEST_F(EventTest, CallbacksAreCalledInOrder)
{
    Event<> event;
    std::vector<int> callOrder;

    event.add([&callOrder]() { callOrder.push_back(1); });
    event.add([&callOrder]() { callOrder.push_back(2); });
    event.add([&callOrder]() { callOrder.push_back(3); });

    event();

    ASSERT_EQ(callOrder.size(), 3);
    EXPECT_EQ(callOrder[0], 1);
    EXPECT_EQ(callOrder[1], 2);
    EXPECT_EQ(callOrder[2], 3);
}


TEST_F(EventTest, CallbackCanBeLambdaWithCapture)
{
    Event<int> event;
    int capturedValue = 100;

    event.add([capturedValue](const int value)
        {
            EXPECT_EQ(value, capturedValue);
        });

    event(100);
}


TEST_F(EventTest, CallbackCanBeFunctionPointer)
{
    Event<> event;
    static int callCount = 0;

    auto function = []() { callCount++; };

    event.add(function);
    event();

    EXPECT_EQ(callCount, 1);
}


TEST_F(EventTest, RemoveCallbackDuringTrigger)
{
    Event<> event;
    MockCallback mock;

    EXPECT_CALL(mock, call()).Times(1);

    callbackId_t idToRemove;

    event.add([&]() { mock.call(); });
    event.add([&event, &idToRemove]() { event.remove(idToRemove); });
    idToRemove = event.add([&mock]() { mock.call(); });  // This one will be removed

    // Should not crash when removing during iteration
    event();
}


TEST_F(EventTest, TriggerWithConstArguments)
{
    Event<const int&> event;
    MockCallback mock;

    EXPECT_CALL(mock, callWithInt(42)).Times(1);

    event.add([&mock](const int& value) { mock.callWithInt(value); });

    constexpr int testValue = 42;
    event(testValue);
}


TEST_F(EventTest, MultipleEventsWithDifferentTypes)
{
    Event<int> intEvent;
    Event<std::string> stringEvent;
    MockCallback mock;

    EXPECT_CALL(mock, callWithInt(42)).Times(1);
    EXPECT_CALL(mock, callWithString("test")).Times(1);

    intEvent.add([&mock](int value) { mock.callWithInt(value); });
    stringEvent.add([&mock](const std::string& value) { mock.callWithString(value); });

    intEvent(42);
    stringEvent("test");
}
