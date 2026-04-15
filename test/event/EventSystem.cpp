#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>
#include "event/EventSystem.hpp"
#include "TrackedObj.hpp"


using namespace event;
using ::testing::_;


class EventSystemMockCallback
{
public:
    MOCK_METHOD(void, call, ());
    MOCK_METHOD(void, callWithInt, (int));
    MOCK_METHOD(void, callWithString, (const std::string&));
    MOCK_METHOD(void, callWithTwoInts, (int, int));
};


class EventSystemTest : public ::testing::Test
{
protected:
    void SetUp() override { system.clear();}

    EventSystem<std::string> system;
};


TEST_F(EventSystemTest, CreateEvent)
{
    EXPECT_TRUE(system.Create<int>("IntEvent"));
    EXPECT_TRUE(system.Create<std::string>("StringEvent"));
    EXPECT_TRUE((system.Create<int, std::string>("ComplexEvent")));
    
    EXPECT_EQ(system.size(), 3);
    EXPECT_TRUE(system.contains("IntEvent"));
    EXPECT_TRUE(system.contains("StringEvent"));
    EXPECT_TRUE(system.contains("ComplexEvent"));
}


TEST_F(EventSystemTest, CreateDuplicateEvent)
{
    EXPECT_TRUE(system.Create<int>("TestEvent"));
    EXPECT_FALSE(system.Create<int>("TestEvent"));  // Should return false
    EXPECT_FALSE(system.Create<std::string>("TestEvent"));  // Different signature, same name
    EXPECT_EQ(system.size(), 1);
}


TEST_F(EventSystemTest, DeleteEvent)
{
    system.Create<int>("TestEvent");
    EXPECT_TRUE(system.contains("TestEvent"));
    
    system.Delete("TestEvent");
    EXPECT_FALSE(system.contains("TestEvent"));
    EXPECT_EQ(system.size(), 0);
}


TEST_F(EventSystemTest, DeleteNonExistentEvent)
{
    EXPECT_NO_THROW(system.Delete("NonExistent"));
    EXPECT_EQ(system.size(), 0);
}


TEST_F(EventSystemTest, OnEventWithCorrectSignature)
{
    EventSystemMockCallback mock;
    
    system.Create<int>("IntEvent");

    auto* event = system.TryGet<int>("IntEvent");
    ASSERT_NE(event, nullptr);
    
    SingleListener<int> listener(
        event,
        [&mock](int value) {mock.callWithInt(value);}
        );
    
    EXPECT_CALL(mock, callWithInt(42)).Times(1);
    system.OnEvent("IntEvent", 42);
}


TEST_F(EventSystemTest, OnEventWithWrongSignature)
{
    EventSystemMockCallback mock;
    
    system.Create<int>("IntEvent");
    
    auto* event = system.TryGet<int>("IntEvent");
    ASSERT_NE(event, nullptr);
    
    SingleListener<int> listener(
        event,
        [&mock](int value) {mock.callWithInt(value);}
        );
    
    // Trigger with wrong signature - should be ignored
    EXPECT_CALL(mock, callWithInt(_)).Times(0);
    system.OnEvent("IntEvent", "wrong", "signature");  // Wrong number/type of args
}


TEST_F(EventSystemTest, OnEventWithNonExistentEvent)
{
    EventSystemMockCallback mock;
    EXPECT_CALL(mock, call()).Times(0);
    
    // Trigger non-existent event - should do nothing
    system.OnEvent("NonExistent");
}


TEST_F(EventSystemTest, MultipleEventsWithSameSignature)
{
    EventSystemMockCallback mock;
    
    system.Create<int>("Event1");
    system.Create<int>("Event2");

    auto* event1 = system.TryGet<int>("Event1");
    auto* event2 = system.TryGet<int>("Event2");
    
    SingleListener<int> listener1(event1, [&mock](int v) { mock.callWithInt(v); });
    SingleListener<int> listener2(event2, [&mock](int v) { mock.callWithInt(v); });
    
    EXPECT_CALL(mock, callWithInt(100)).Times(1);
    EXPECT_CALL(mock, callWithInt(200)).Times(1);
    
    system.OnEvent("Event1", 100);
    system.OnEvent("Event2", 200);
}


enum class EventType
{
    PlayerDamaged,
    PlayerDied,
    ItemCollected,
    GameStarted
};


class EventSystemEnumTest : public ::testing::Test
{
protected:
    void SetUp() override {system.clear(); }

    EventSystem<EventType> system;
};


TEST_F(EventSystemEnumTest, CreateAndTriggerWithEnumKeys)
{
    EventSystemMockCallback mock;
    
    system.Create<int>(EventType::PlayerDamaged);
    system.Create<std::string>(EventType::PlayerDied);
    system.Create<int, int>(EventType::ItemCollected);
    
    EXPECT_TRUE(system.contains(EventType::PlayerDamaged));
    EXPECT_TRUE(system.contains(EventType::PlayerDied));
    EXPECT_TRUE(system.contains(EventType::ItemCollected));
    EXPECT_FALSE(system.contains(EventType::GameStarted));
    
    // Setup listeners
    auto* damageEvent = system.TryGet<int>(EventType::PlayerDamaged);
    auto* deathEvent = system.TryGet<std::string>(EventType::PlayerDied);

    SingleListener<int> damageListener(damageEvent, 
        [&mock](int v) { mock.callWithInt(v); });
    SingleListener<std::string> deathListener(
        deathEvent,
        [&mock](const std::string& v) { mock.callWithString(v); }
        );
    
    EXPECT_CALL(mock, callWithInt(50)).Times(1);
    EXPECT_CALL(mock, callWithString("Game Over")).Times(1);
    
    system.OnEvent<int>(EventType::PlayerDamaged, 50);
    system.OnEvent<std::string>(EventType::PlayerDied, "Game Over");
}


TEST_F(EventSystemTest, IntegerKeys)
{
    EventSystem<int> intSystem;
    EventSystemMockCallback mock;
    
    intSystem.Create<int>(1);
    intSystem.Create<std::string>(2);
    
    auto* event1 = intSystem.TryGet<int>(1);
    auto* event2 = intSystem.TryGet<std::string>(2);
    
    SingleListener<int> listener1(event1, [&mock](int v) { mock.callWithInt(v); });
    SingleListener<std::string> listener2(event2, [&mock](const std::string& v) { mock.callWithString(v); });
    
    EXPECT_CALL(mock, callWithInt(42)).Times(1);
    EXPECT_CALL(mock, callWithString("test")).Times(1);
    
    intSystem.OnEvent<int>(1, 42);
    intSystem.OnEvent<std::string>(2, "test");
}


TEST_F(EventSystemTest, Clear)
{
    system.Create<int>("Event1");
    system.Create<std::string>("Event2");
    
    EXPECT_EQ(system.size(), 2);
    
    system.clear();
    EXPECT_EQ(system.size(), 0);
    EXPECT_FALSE(system.contains("Event1"));
    EXPECT_FALSE(system.contains("Event2"));
}


TEST_F(EventSystemTest, ComplexEventTypes)
{
    EventSystemMockCallback mock;
    
    system.Create<int, std::string, double>("ComplexEvent");
    
    auto* event = system.TryGet<int, std::string, double>("ComplexEvent");
    
    SingleListener<int, std::string, double> listener(event,
        [&mock](int i, const std::string& s, double d)
        {
            mock.callWithInt(i);
            mock.callWithString(s);
        });
    
    EXPECT_CALL(mock, callWithInt(42)).Times(1);
    EXPECT_CALL(mock, callWithString("test")).Times(1);
    
    system.OnEvent<int, std::string, double>("ComplexEvent", 42, "test", 3.14);
}


TEST_F(EventSystemTest, PerfectForwarding)
{
    struct MoveOnlyType
    {
        explicit MoveOnlyType(int v) : value(v) {}
        MoveOnlyType(const MoveOnlyType&) = delete;
        MoveOnlyType& operator=(const MoveOnlyType&) = delete;
        MoveOnlyType(MoveOnlyType&&) = default;
        MoveOnlyType& operator=(MoveOnlyType&&) = default;
        int value;
    };
    
    system.Create<MoveOnlyType>("MoveOnlyEvent");
    
    auto* event = system.TryGet<MoveOnlyType>("MoveOnlyEvent");
    
    bool callbackCalled = false;
    SingleListener<MoveOnlyType> listener(event,
        [&callbackCalled](MoveOnlyType&& mot)
        {
            EXPECT_EQ(mot.value, 100);
            callbackCalled = true;
        });
    
    MoveOnlyType mot(100);
    system.OnEvent("MoveOnlyEvent", std::move(mot));
    
    EXPECT_TRUE(callbackCalled);
}


TEST_F(EventSystemTest, PushEvent_DoesNotTriggerImmediately)
{
    EventSystemMockCallback mock;

    system.Create<>("test");

    EXPECT_CALL(mock, call()).Times(0);

    system.Get<>("test").add([&]() { mock.call(); });

    system.PushEvent("test");
}


TEST_F(EventSystemTest, FlushEvents_TriggersQueuedEvents)
{
    EventSystemMockCallback mock;

    system.Create<>("test");

    EXPECT_CALL(mock, call()).Times(1);

    system.Get<>("test").add([&]() { mock.call(); });

    system.PushEvent("test");
    system.FlushEvents();
}


TEST_F(EventSystemTest, FlushEvents_ProcessesEventsInOrder)
{
    std::vector<int> order;

    system.Create<>("e1");
    system.Create<>("e2");

    system.Get<>("e1").add([&]() { order.push_back(1); });
    system.Get<>("e2").add([&]() { order.push_back(2); });

    system.PushEvent("e1");
    system.PushEvent("e2");

    system.FlushEvents();

    ASSERT_EQ(order.size(), 2);
    EXPECT_EQ(order[0], 1);
    EXPECT_EQ(order[1], 2);
}


TEST_F(EventSystemTest, FlushEvents_ClearsQueue)
{
    EventSystemMockCallback mock;

    system.Create<>("test");

    EXPECT_CALL(mock, call()).Times(1);

    system.Get<>("test").add([&]() { mock.call(); });

    system.PushEvent("test");
    system.FlushEvents();

    system.FlushEvents();
}


TEST_F(EventSystemTest, PushEvent_WithArguments)
{
    EventSystemMockCallback mock;

    system.Create<int>("test");

    EXPECT_CALL(mock, callWithInt(42)).Times(1);

    system.Get<int>("test").add(
        [&](int v) { mock.callWithInt(v); });

    system.PushEvent("test", 42);
    system.FlushEvents();
}


TEST_F(EventSystemTest, PushEvent_WithMultipleArguments)
{
    EventSystemMockCallback mock;

    system.Create<int, int>("test");

    EXPECT_CALL(mock, callWithTwoInts(10, 20)).Times(1);

    system.Get<int, int>("test").add(
        [&](int a, int b) { mock.callWithTwoInts(a, b); });

    system.PushEvent("test", 10, 20);
    system.FlushEvents();
}


TEST_F(EventSystemTest, PushEvent_NonExistentEvent_DoesNothing)
{
    EXPECT_NO_THROW(system.PushEvent("unknown"));
    EXPECT_NO_THROW(system.FlushEvents());
}


TEST_F(EventSystemTest, PushEvent_DuringFlush_ExecutedNextFlush)
{
    std::vector<int> order;

    system.Create<>("test");

    system.Get<>("test").add([&]()
    {
        order.push_back(1);
        system.PushEvent("test"); // enqueue during flush
    });

    system.PushEvent("test");
    system.FlushEvents();

    ASSERT_EQ(order.size(), 1);

    system.FlushEvents();

    ASSERT_EQ(order.size(), 2);
}


TEST_F(EventSystemTest, PushEvent_NoExtraCopies)
{
    TrackedObj::Reset();

    system.Create<TrackedObj>("test");

    EventSystemMockCallback mock;

    EXPECT_CALL(mock, call()).Times(1);

    system.Get<TrackedObj>("test").add([&](TrackedObj t)
    {
        mock.call();
    });

    TrackedObj obj(42);

    system.PushEvent("test", obj);   // <-- lvalue
    system.FlushEvents();


    EXPECT_LE(TrackedObj::copyCount, 1);
}


TEST_F(EventSystemTest, PushEvent_UsesMoveForPValue)
{
    TrackedObj::Reset();

    system.Create<TrackedObj>("test");

    EventSystemMockCallback mock;

    EXPECT_CALL(mock, call()).Times(1);

    system.Get<TrackedObj>("test").add([&](TrackedObj t)
    {
        mock.call();
    });

    system.PushEvent("test", TrackedObj(42));  // pvalue
    system.FlushEvents();

    EXPECT_EQ(TrackedObj::copyCount, 0);
    EXPECT_GE(TrackedObj::moveCount, 1);
}