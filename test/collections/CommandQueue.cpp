#include <gtest/gtest.h>
#include "collections/CommandQueue.hpp"


using namespace collections;

class CommandQueueTest : public ::testing::Test
{
protected:
    CommandQueue queue;
};


TEST_F(CommandQueueTest, InitiallyEmpty)
{
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), 0);
}


TEST_F(CommandQueueTest, PushIncreasesSize)
{
    queue.Push([] {});
    EXPECT_FALSE(queue.empty());
    EXPECT_EQ(queue.size(), 1);

    queue.Push([] {});
    EXPECT_EQ(queue.size(), 2);
}


TEST_F(CommandQueueTest, FlushExecutesInFifoOrder)
{
    std::vector<int> order;

    queue.Push([&] { order.push_back(1); });
    queue.Push([&] { order.push_back(2); });
    queue.Push([&] { order.push_back(3); });

    queue.Flush();

    ASSERT_EQ(order.size(), 3);
    EXPECT_EQ(order[0], 1);
    EXPECT_EQ(order[1], 2);
    EXPECT_EQ(order[2], 3);
}


TEST_F(CommandQueueTest, FlushClearsQueue)
{
    queue.Push([] {});
    queue.Push([] {});
    queue.Flush();

    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), 0);
}


TEST_F(CommandQueueTest, CommandsPushedDuringFlushAreDeferred)
{
    std::vector<int> order;

    queue.Push([&]
    {
        order.push_back(1);
        queue.Push([&] { order.push_back(3); });
    });
    queue.Push([&] { order.push_back(2); });

    queue.Flush();

    ASSERT_EQ(order.size(), 2);
    EXPECT_EQ(order[0], 1);
    EXPECT_EQ(order[1], 2);

    queue.Flush();

    ASSERT_EQ(order.size(), 3);
    EXPECT_EQ(order[2], 3);
}


TEST_F(CommandQueueTest, MultipleFlushesWork)
{
    int value = 0;

    queue.Push([&] { value += 1; });
    queue.Flush();
    EXPECT_EQ(value, 1);

    queue.Push([&] { value += 10; });
    queue.Flush();
    EXPECT_EQ(value, 11);

    queue.Push([&] { value += 100; });
    queue.Flush();
    EXPECT_EQ(value, 111);
}


TEST_F(CommandQueueTest, FlushOnEmptyQueueDoesNothing)
{
    EXPECT_NO_THROW(queue.Flush());
    EXPECT_TRUE(queue.empty());
}


TEST_F(CommandQueueTest, MoveConstructorTransfersCommands)
{
    queue.Push([] {});
    queue.Push([] {});
    EXPECT_EQ(queue.size(), 2);

    CommandQueue other(std::move(queue));
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(other.size(), 2);
}

TEST_F(CommandQueueTest, MoveAssignmentTransfersCommands)
{
    queue.Push([] {});
    const CommandQueue other = std::move(queue);

    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(other.size(), 1);
}


TEST_F(CommandQueueTest, MoveDoesNotBreakExecution)
{
    CommandQueue other;
    {
        CommandQueue temp;
        temp.Push([] {});
        temp.Push([] {});
        other = std::move(temp);
    }

    EXPECT_EQ(other.size(), 2);

    int count = 0;
    other.Push([&] { count = 42; });
    other.Flush();
    EXPECT_EQ(count, 42);
}


TEST_F(CommandQueueTest, CapturedStateIsPreserved)
{
    int a = 10;
    int b = 20;

    queue.Push([&] { a = a + b; });
    queue.Push([&] { b = b + a; });
    queue.Flush();

    EXPECT_EQ(a, 30);
    EXPECT_EQ(b, 50);
}


TEST_F(CommandQueueTest, MultipleQueuesAreIndependent)
{
    CommandQueue q1, q2;
    int v1 = 0, v2 = 0;

    q1.Push([&] { v1 = 1; });
    q2.Push([&] { v2 = 2; });

    q1.Flush();
    EXPECT_EQ(v1, 1);
    EXPECT_EQ(v2, 0);

    q2.Flush();
    EXPECT_EQ(v2, 2);
}


struct Incrementor
{
    int& target;
    void operator()() const { ++target; }
};


TEST_F(CommandQueueTest, FunctorWithOperatorCall)
{
    int value = 0;
    queue.Push(Incrementor{value});
    queue.Flush();
    EXPECT_EQ(value, 1);
}


struct Accumulator
{
    int& sum;
    int addend;
    void operator()() const { sum += addend; }
};

TEST_F(CommandQueueTest, MultipleFunctorsWithState)
{
    int total = 0;
    queue.Push(Accumulator{total, 10});
    queue.Push(Accumulator{total, 20});
    queue.Push(Accumulator{total, 30});
    queue.Flush();
    EXPECT_EQ(total, 60);
}


struct CopyTracker
{
    int& copyCount;
    int& moveCount;
    CopyTracker(int& cc, int& mc) : copyCount(cc), moveCount(mc) {}
    CopyTracker(const CopyTracker& o) : copyCount(o.copyCount), moveCount(o.moveCount) { ++copyCount; }
    CopyTracker(CopyTracker&& o) noexcept : copyCount(o.copyCount), moveCount(o.moveCount) { ++moveCount; }
    void operator()() const {}
};

TEST_F(CommandQueueTest, FunctorCopiedIntoQueue)
{
    int copies = 0, moves = 0;
    {
        CopyTracker tracker(copies, moves);
        queue.Push(tracker);
    }
    EXPECT_GE(copies, 1);
}


TEST_F(CommandQueueTest, FunctorMovedIntoQueue)
{
    int copies = 0, moves = 0;
    queue.Push(CopyTracker(copies, moves));
    EXPECT_GE(moves, 1);
}


struct CountCalls
{
    int& count;
    void operator()() const { ++count; }
};

TEST_F(CommandQueueTest, FunctorPreservedAcrossFlushCycles)
{
    int count = 0;
    queue.Push(CountCalls{count});
    queue.Flush();
    EXPECT_EQ(count, 1);

    queue.Push(CountCalls{count});
    queue.Flush();
    EXPECT_EQ(count, 2);
}


struct FunctionPointer
{
    static void call() {}
};

TEST_F(CommandQueueTest, FunctionPointerAccepted)
{
    bool called = false;

    auto lambda = [&] { called = true; };

    queue.Push(lambda);
    queue.Flush();

    EXPECT_TRUE(called);
}
