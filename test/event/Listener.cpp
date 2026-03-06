#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>
#include <memory>
#include "event/Event.hpp"
#include "event/Listener.hpp"


using namespace event;
using ::testing::_;
using ::testing::Exactly;
using ::testing::Invoke;
using ::testing::Return;


// Mock classes for testing
class ListenerMockCallback
{
public:
    MOCK_METHOD(void, call, ());
    MOCK_METHOD(void, callWithInt, (int));
    MOCK_METHOD(void, callWithString, (const std::string&));
    MOCK_METHOD(void, callWithTwoInts, (int, int));
};

// Test fixture for Listener tests with single callback ID
class ListenerSingleTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        intEvent = std::make_unique<Event<int>>();
        stringEvent = std::make_unique<Event<std::string>>();
        multiArgEvent = std::make_unique<Event<int, std::string>>();
        callCount = 0;
    }

    void TearDown() override
    {
        // Reset events
        intEvent.reset();
        stringEvent.reset();
        multiArgEvent.reset();
    }

    // Helper method to create a callback that increments counter
    auto createCounterCallback()
    {
        return [this]() { callCount++; };
    }

    std::unique_ptr<Event<int>> intEvent;
    std::unique_ptr<Event<std::string>> stringEvent;
    std::unique_ptr<Event<int, std::string>> multiArgEvent;
    int callCount = 0;
};


TEST_F(ListenerSingleTest, DefaultConstructorCreatesEmptyListener)
{
    Listener<callbackId_t, int> listener;
    EXPECT_EQ(listener.eventId(), INVALID_EVENT_ID);
}


TEST_F(ListenerSingleTest, ConstructorWithEventOnly)
{
    Listener<callbackId_t, int> listener(intEvent.get());
    EXPECT_EQ(listener.eventId(), intEvent->id);
}

#include "iostream"
TEST_F(ListenerSingleTest, ConstructorWithEventAndCallback)
{
    ListenerMockCallback mock;
    EXPECT_CALL(mock, callWithInt(42)).Times(1);

    {
        Listener<callbackId_t, int> listener(
        intEvent.get(),
        [&mock](const int value) { mock.callWithInt(value); }
        );

        (*intEvent)(42);
    }
}


TEST_F(ListenerSingleTest, ConstructorWithEventCallbackAndDeleter)
{
    bool deleterCalled = false;

    {
        Listener<callbackId_t, int> listener(
            intEvent.get(),
            [](int) {},
            [&deleterCalled](callbackId_t) { deleterCalled = true; }
            );
    }

    EXPECT_TRUE(deleterCalled);
}


TEST_F(ListenerSingleTest, MoveConstructorTransfersCallbacks)
{
    ListenerMockCallback mock;
    EXPECT_CALL(mock, callWithInt(42)).Times(1);

    Listener<callbackId_t, int> source(
        intEvent.get(),
        [&mock](const int value) { mock.callWithInt(value); }
        );

    Listener<callbackId_t, int> dest(std::move(source));

    // Source should be empty
    EXPECT_EQ(source.eventId(), 0);

    // Dest should handle the callback
    (*intEvent)(42);
}


TEST_F(ListenerSingleTest, MoveAssignmentTransfersCallbacks)
{
    ListenerMockCallback mock;
    EXPECT_CALL(mock, callWithInt(42)).Times(1);

    Listener<callbackId_t, int> source(
        intEvent.get(),
        [&mock](int value) { mock.callWithInt(value); }
        );

    Listener<callbackId_t, int> dest;
    dest = std::move(source);

    // Source should be empty
    EXPECT_EQ(source.eventId(), 0);

    // Dest should handle the callback
    (*intEvent)(42);
}


TEST_F(ListenerSingleTest, MoveFromListenerDoesntAffectExistingCallbacks)
{
    ListenerMockCallback mock1, mock2;
    EXPECT_CALL(mock1, callWithInt(42)).Times(1);
    EXPECT_CALL(mock2, callWithInt(42)).Times(1);

    Listener<callbackId_t, int> listener1(intEvent.get(),
        [&mock1](int value) { mock1.callWithInt(value); });

    Listener<callbackId_t, int> listener2(intEvent.get(),
        [&mock2](int value) { mock2.callWithInt(value); });

    Listener<callbackId_t, int> dest(std::move(listener1));

    // Both original and moved callbacks should work
    (*intEvent)(42);
}


TEST_F(ListenerSingleTest, SwapExchangesContents)
{
    ListenerMockCallback mock1, mock2;
    auto event2 = std::make_unique<Event<int>>();

    Listener<callbackId_t, int> listener1(intEvent.get(),
        [&mock1](int value) { mock1.callWithInt(value); });

    Listener<callbackId_t, int> listener2(event2.get(),
        [&mock2](int value) { mock2.callWithInt(value); });

    eventId_t id1 = listener1.eventId();
    eventId_t id2 = listener2.eventId();

    listener1.swap(std::move(listener2));

    // After swap, listener1 should be associated with event2
    EXPECT_EQ(listener1.eventId(), id2);

    // Callbacks should be swapped
    EXPECT_CALL(mock1, callWithInt(100)).Times(1);
    EXPECT_CALL(mock2, callWithInt(200)).Times(1);

    (*intEvent)(100);  // Should call mock2
    (*event2)(200);    // Should call mock1
}


TEST_F(ListenerSingleTest, AddCallbackAfterConstruction)
{
    ListenerMockCallback mock;
    EXPECT_CALL(mock, callWithInt(42)).Times(1);

    Listener<callbackId_t, int> listener(intEvent.get());
    listener.subscribe([&mock](int value) { mock.callWithInt(value); });

    (*intEvent)(42);
}


TEST_F(ListenerSingleTest, AddMultipleCallbacks)
{
    ListenerMockCallback mock1, mock2, mock3;
    EXPECT_CALL(mock1, callWithInt(42)).Times(0);
    EXPECT_CALL(mock2, callWithInt(42)).Times(0);
    EXPECT_CALL(mock3, callWithInt(42)).Times(1);

    Listener<callbackId_t, int> listener(intEvent.get());
    listener.subscribe([&mock1](int value) { mock1.callWithInt(value); });
    listener.subscribe([&mock2](int value) { mock2.callWithInt(value); });
    listener.subscribe([&mock3](int value) { mock3.callWithInt(value); });

    (*intEvent)(42);
}


TEST_F(ListenerSingleTest, EventIdReturnsCorrectValue)
{
    Listener<callbackId_t, int> listener(intEvent.get());
    EXPECT_EQ(listener.eventId(), intEvent->id);
}


TEST_F(ListenerSingleTest, EventIdReturnsZeroWhenNoEvent)
{
    Listener<callbackId_t, int> listener;
    EXPECT_EQ(listener.eventId(), 0);
}


TEST_F(ListenerSingleTest, DestructorRemovesCallbacks)
{
    ListenerMockCallback mock;
    EXPECT_CALL(mock, callWithInt(_)).Times(0);

    {
        Listener<callbackId_t, int> listener(intEvent.get(),
            [&mock](int value) { mock.callWithInt(value); });
    }

    // Callback should be removed after listener destruction
    (*intEvent)(42);
}


TEST_F(ListenerSingleTest, MultipleListenersDestruction)
{
    ListenerMockCallback mock1, mock2, mock3;

    {
        Listener<callbackId_t, int> listener1(
            intEvent.get(),
            [&mock1](int value) { mock1.callWithInt(value); }
            );

        Listener<callbackId_t, int> listener2(
            intEvent.get(),
            [&mock2](int value) { mock2.callWithInt(value); }
            );

        EXPECT_CALL(mock1, callWithInt(42)).Times(1);
        EXPECT_CALL(mock2, callWithInt(42)).Times(1);
        (*intEvent)(42);
    }

    // After both destroyed, no callbacks should be called
    EXPECT_CALL(mock1, callWithInt(_)).Times(0);
    EXPECT_CALL(mock2, callWithInt(_)).Times(0);
    EXPECT_CALL(mock3, callWithInt(_)).Times(0);

    Listener<callbackId_t, int> listener3(
        intEvent.get(),
        [&mock3](const int value) { mock3.callWithInt(value); }
        );

    // Only listener3's callback should be called
    EXPECT_CALL(mock3, callWithInt(100)).Times(1);
    (*intEvent)(100);
}


TEST_F(ListenerSingleTest, WorksWithStringEvent) {
    ListenerMockCallback mock;
    EXPECT_CALL(mock, callWithString("Hello")).Times(1);

    Listener<callbackId_t, std::string> listener(stringEvent.get(),
        [&mock](const std::string& str) { mock.callWithString(str); });

    (*stringEvent)("Hello");
}


TEST_F(ListenerSingleTest, WorksWithMultipleArguments)
{
    ListenerMockCallback mock;
    EXPECT_CALL(mock, callWithTwoInts(10, 20)).Times(1);

    auto event = std::make_unique<Event<int, int>>();
    Listener<callbackId_t, int, int> listener(event.get(),
        [&mock](int a, int b) { mock.callWithTwoInts(a, b); });

    (*event)(10, 20);
}


TEST_F(ListenerSingleTest, ListenerWithNullEvent)
{
    Listener<callbackId_t, int> listener(nullptr);
    EXPECT_EQ(listener.eventId(), 0);

    // Adding callback to null event should not crash
    listener.subscribe([](int) {});
}


TEST_F(ListenerSingleTest, MultipleListenersSameEvent)
{
    ListenerMockCallback mock1, mock2, mock3;

    Listener<callbackId_t, int> listener1(intEvent.get(),
        [&mock1](int value) { mock1.callWithInt(value); });

    Listener<callbackId_t, int> listener2(intEvent.get(),
        [&mock2](int value) { mock2.callWithInt(value); });

    Listener<callbackId_t, int> listener3(intEvent.get(),
        [&mock3](int value) { mock3.callWithInt(value); });

    EXPECT_CALL(mock1, callWithInt(42)).Times(1);
    EXPECT_CALL(mock2, callWithInt(42)).Times(1);
    EXPECT_CALL(mock3, callWithInt(42)).Times(1);

    (*intEvent)(42);
}


class ListenerVectorTest : public ::testing::Test
{
protected:
    void SetUp() override {
        intEvent = std::make_unique<Event<int>>();
    }

    std::unique_ptr<Event<int>> intEvent;
};


TEST_F(ListenerVectorTest, ConstructorWithMultipleCallbacks)
{
    ListenerMockCallback mock1, mock2, mock3;
    std::vector<eventCallback_t<int>> callbacks;

    callbacks.push_back([&mock1](int value) { mock1.callWithInt(value); });
    callbacks.push_back([&mock2](int value) { mock2.callWithInt(value); });
    callbacks.push_back([&mock3](int value) { mock3.callWithInt(value); });

    Listener<std::vector<callbackId_t>, int> listener(intEvent.get(), std::move(callbacks));

    EXPECT_CALL(mock1, callWithInt(42)).Times(1);
    EXPECT_CALL(mock2, callWithInt(42)).Times(1);
    EXPECT_CALL(mock3, callWithInt(42)).Times(1);

    (*intEvent)(42);
}


TEST_F(ListenerVectorTest, ConstructorWithMultipleCallbacksAndDeleter)
{
    bool deleterCalled = false;
    std::vector<eventCallback_t<int>> callbacks;

    callbacks.push_back([](int) {});
    callbacks.push_back([](int) {});

    {
        Listener<std::vector<callbackId_t>, int> listener(
            intEvent.get(),
            std::move(callbacks),
            [&deleterCalled](callbackId_t) { deleterCalled = true; }
        );
    }

    EXPECT_TRUE(deleterCalled);
}


TEST_F(ListenerVectorTest, AddCallbackToVectorListener)
{
    ListenerMockCallback mock1, mock2;

    Listener<std::vector<callbackId_t>, int> listener(intEvent.get());

    listener.subscribe([&mock1](int value) { mock1.callWithInt(value); });
    listener.subscribe([&mock2](int value) { mock2.callWithInt(value); });

    EXPECT_CALL(mock1, callWithInt(42)).Times(1);
    EXPECT_CALL(mock2, callWithInt(42)).Times(1);

    (*intEvent)(42);
}


TEST_F(ListenerVectorTest, MoveVectorListener)
{
    ListenerMockCallback mock;
    std::vector<eventCallback_t<int>> callbacks;
    callbacks.push_back([&mock](const int value) { mock.callWithInt(value); });

    Listener<std::vector<callbackId_t>, int> source(
        intEvent.get(), std::move(callbacks));

    Listener<std::vector<callbackId_t>, int> dest(std::move(source));

    EXPECT_CALL(mock, callWithInt(42)).Times(1);
    (*intEvent)(42);
}

TEST_F(ListenerVectorTest, UnionCallbacksTransfersCallbacks)
{
    ListenerMockCallback mock1, mock2;

    Listener<std::vector<callbackId_t>, int> listener1(
        intEvent.get(),
        [&mock1](const int value) { mock1.callWithInt(value); }
        );

    Listener<std::vector<callbackId_t>, int> listener2(
        intEvent.get(),
        [&mock2](const int value) { mock2.callWithInt(value); }
        );

    listener1.unionCallbacks(std::move(listener2));

    // Both callbacks should be in listener1 now
    EXPECT_CALL(mock1, callWithInt(42)).Times(1);
    EXPECT_CALL(mock2, callWithInt(42)).Times(1);

    (*intEvent)(42);

    // listener2 should be empty
    EXPECT_EQ(listener2.eventId(), INVALID_EVENT_ID);
}


TEST_F(ListenerSingleTest, ListenersCanBeStoredInContainer)
{
    std::vector<Listener<callbackId_t, int>> listeners;
    ListenerMockCallback mock1, mock2;

    listeners.emplace_back(intEvent.get(),
        [&mock1](int value) { mock1.callWithInt(value); });
    listeners.emplace_back(intEvent.get(),
        [&mock2](int value) { mock2.callWithInt(value); });

    EXPECT_CALL(mock1, callWithInt(42)).Times(1);
    EXPECT_CALL(mock2, callWithInt(42)).Times(1);

    (*intEvent)(42);
}


TEST_F(ListenerSingleTest, ListenerWorksWithLambdaCaptures)
{
    int capturedValue = 100;
    bool callbackExecuted = false;

    Listener<callbackId_t, int> listener(intEvent.get(),
        [capturedValue, &callbackExecuted](const int value)
        {
            EXPECT_EQ(value, capturedValue);
            callbackExecuted = true;
        });

    (*intEvent)(100);
    EXPECT_TRUE(callbackExecuted);
}
