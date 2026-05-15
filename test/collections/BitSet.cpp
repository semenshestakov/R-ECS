#include <gtest/gtest.h>
#include "collections/BitSet.hpp"


using collection::BitSet;


TEST(BitSetTest, ConstructorAndSize)
{
    BitSet bs(100);
    EXPECT_EQ(bs.size(), 100);
    EXPECT_FALSE(bs.empty());
}


TEST(BitSetTest, EmptyBitSet)
{
    BitSet bs(0);
    EXPECT_TRUE(bs.empty());
    EXPECT_EQ(bs.size(), 0);
}


TEST(BitSetTest, SetAndTestBasic)
{
    BitSet bs(64);

    bs.set(0);
    bs.set(10);
    bs.set(63);

    EXPECT_TRUE(bs.test(0));
    EXPECT_TRUE(bs.test(10));
    EXPECT_TRUE(bs.test(63));

    EXPECT_FALSE(bs.test(1));
    EXPECT_FALSE(bs.test(62));
}


TEST(BitSetTest, ResetWorks)
{
    BitSet bs(64);

    bs.set(5);
    EXPECT_TRUE(bs.test(5));

    bs.reset(5);
    EXPECT_FALSE(bs.test(5));
}


TEST(BitSetTest, OutOfRangeSetDoesNothing)
{
    BitSet bs(10);

    bs.set(100);
    EXPECT_FALSE(bs.test(100));
}


TEST(BitSetTest, OutOfRangeTestReturnsFalse)
{
    BitSet bs(10);
    EXPECT_FALSE(bs.test(999));
}


TEST(BitSetTest, OperatorSquareBrackets)
{
    BitSet bs(64);

    bs.set(7);

    EXPECT_TRUE(bs[7]);
    EXPECT_FALSE(bs[8]);

    const BitSet& ref = bs;
    EXPECT_TRUE(ref[7]);
}


TEST(BitSetTest, SetAllAndResetAll)
{
    BitSet bs(100);

    bs.set_all();

    for (size_t i = 0; i < 100; ++i)
        EXPECT_TRUE(bs.test(i));

    bs.reset_all();

    for (size_t i = 0; i < 100; ++i)
        EXPECT_FALSE(bs.test(i));
}


TEST(BitSetTest, FlipWorks)
{
    BitSet bs(10);

    bs.set(1);
    bs.set(3);

    bs.flip();

    EXPECT_TRUE(bs.test(0));
    EXPECT_FALSE(bs.test(1));
    EXPECT_TRUE(bs.test(2));
    EXPECT_FALSE(bs.test(3));
}


TEST(BitSetTest, CopyAndEquality)
{
    BitSet a(64);
    a.set(1);
    a.set(2);

    BitSet b = a;

    EXPECT_EQ(a, b);

    b.reset(2);

    EXPECT_NE(a, b);
}


TEST(BitSetTest, NotEqualDifferentSize)
{
    BitSet a(10);
    BitSet b(20);

    EXPECT_NE(a, b);
}


TEST(BitSetTest, OR_Operator)
{
    BitSet a(64), b(64);

    a.set(1);
    b.set(2);

    BitSet c = a | b;

    EXPECT_TRUE(c.test(1));
    EXPECT_TRUE(c.test(2));
}


TEST(BitSetTest, AND_Operator)
{
    BitSet a(64), b(64);

    a.set(1);
    a.set(2);

    b.set(2);

    BitSet c = a & b;

    EXPECT_FALSE(c.test(1));
    EXPECT_TRUE(c.test(2));
}


TEST(BitSetTest, XOR_Operator)
{
    BitSet a(64), b(64);

    a.set(1);
    a.set(2);

    b.set(2);
    b.set(3);

    BitSet c = a ^ b;

    EXPECT_TRUE(c.test(1));
    EXPECT_FALSE(c.test(2));
    EXPECT_TRUE(c.test(3));
}


TEST(BitSetTest, OR_Assignment)
{
    BitSet a(64), b(64);

    a.set(1);
    b.set(2);

    a |= b;

    EXPECT_TRUE(a.test(1));
    EXPECT_TRUE(a.test(2));
}


TEST(BitSetTest, AND_Assignment)
{
    BitSet a(64), b(64);

    a.set(1);
    a.set(2);

    b.set(2);

    a &= b;

    EXPECT_FALSE(a.test(1));
    EXPECT_TRUE(a.test(2));
}


TEST(BitSetTest, XOR_Assignment)
{
    BitSet a(64), b(64);

    a.set(1);
    a.set(2);
    b.set(2);
    b.set(3);

    a ^= b;

    EXPECT_TRUE(a.test(1));
    EXPECT_FALSE(a.test(2));
    EXPECT_TRUE(a.test(3));
}


TEST(BitSetTest, NotOperator)
{
    BitSet bs(4);

    bs.set(1);
    bs.set(3);

    BitSet inv = ~bs;

    EXPECT_TRUE(inv.test(0));
    EXPECT_FALSE(inv.test(1));
    EXPECT_TRUE(inv.test(2));
    EXPECT_FALSE(inv.test(3));
}


TEST(BitSetTest, LastBitsMasking)
{
    BitSet bs(70);

    bs.set_all();

    for (size_t i = 0; i < 70; ++i)
        EXPECT_TRUE(bs.test(i));

    EXPECT_FALSE(bs.test(1000));
}


TEST(BitSetTest, FlipPreservesMask)
{
    BitSet bs(70);

    bs.set_all();
    bs.flip();

    for (size_t i = 0; i < 70; ++i)
        EXPECT_FALSE(bs.test(i));
}


TEST(BitSetTest, SelfAssignmentDoesNotBreak)
{
    BitSet bs(64);

    bs.set(1);
    bs.set(10);

    bs = bs;

    EXPECT_TRUE(bs.test(1));
    EXPECT_TRUE(bs.test(10));
}

TEST(BitSetTest, SelfBitwiseAndAssignment)
{
    BitSet bs(64);

    bs.set(3);
    bs.set(7);

    bs &= bs;

    EXPECT_TRUE(bs.test(3));
    EXPECT_TRUE(bs.test(7));
}


TEST(BitSetTest, SelfBitwiseOrAssignment)
{
    BitSet bs(64);

    bs.set(5);

    bs |= bs;

    EXPECT_TRUE(bs.test(5));
}


TEST(BitSetTest, SelfBitwiseXorAssignment_ShouldZeroOut)
{
    BitSet bs(64);

    bs.set(2);
    bs.set(4);

    bs ^= bs;

    EXPECT_FALSE(bs.test(2));
    EXPECT_FALSE(bs.test(4));
}


TEST(BitSetTest, MultipleSetResetPatternStability)
{
    BitSet bs(128);

    for (int i = 0; i < 128; i += 2)
        bs.set(i);

    for (int i = 0; i < 128; i += 4)
        bs.reset(i);

    for (int i = 0; i < 128; ++i)
    {
        if (i % 4 == 0)
            EXPECT_FALSE(bs.test(i));
        else if (i % 2 == 0)
            EXPECT_TRUE(bs.test(i));
        else
            EXPECT_FALSE(bs.test(i));
    }
}


TEST(BitSetTest, CrossBoundaryBitManipulation)
{
    BitSet bs(130);

    bs.set(63);
    bs.set(64);
    bs.set(65);

    EXPECT_TRUE(bs.test(63));
    EXPECT_TRUE(bs.test(64));
    EXPECT_TRUE(bs.test(65));

    bs.reset(64);

    EXPECT_TRUE(bs.test(63));
    EXPECT_FALSE(bs.test(64));
    EXPECT_TRUE(bs.test(65));
}


TEST(BitSetTest, BitConsistencyAfterFlipTwice)
{
    BitSet bs(100);

    bs.set(1);
    bs.set(50);

    BitSet copy = bs;

    bs.flip();
    bs.flip();

    EXPECT_EQ(bs, copy);
}


TEST(BitSetTest, ChainOperationsDoNotCorruptState)
{
    BitSet a(64), b(64), c(64);

    a.set(1);
    b.set(2);
    c.set(3);

    BitSet result = (a | b) ^ c;

    EXPECT_TRUE(result.test(1));
    EXPECT_TRUE(result.test(2));
    EXPECT_TRUE(result.test(3));
}


TEST(BitSetTest, SetAllThenResetSingleBit)
{
    BitSet bs(80);

    bs.set_all();
    bs.reset(10);

    EXPECT_FALSE(bs.test(10));

    for (size_t i = 0; i < 80; ++i)
    {
        if (i != 10)
            EXPECT_TRUE(bs.test(i));
    }
}


TEST(BitSetTest, LargeBitsetStressPattern)
{
    BitSet bs(1000);

    for (size_t i = 0; i < 1000; i += 3)
        bs.set(i);

    for (size_t i = 0; i < 1000; i += 5)
        bs.reset(i);

    for (size_t i = 0; i < 1000; ++i)
    {
        bool expected = (i % 3 == 0) && (i % 5 != 0);
        EXPECT_EQ(bs.test(i), expected);
    }
}


TEST(BitSetTest, OperatorNotDoesNotModifyOriginal)
{
    BitSet bs(64);

    bs.set(1);
    bs.set(2);

    BitSet copy = bs;

    auto inv = ~bs;

    EXPECT_EQ(bs, copy); // оригинал не изменился
    EXPECT_NE(inv, bs);
}


TEST(BitSetTest, MultipleResetIdempotence)
{
    BitSet bs(64);

    bs.set(10);

    bs.reset(10);
    bs.reset(10);
    bs.reset(10);

    EXPECT_FALSE(bs.test(10));
}
