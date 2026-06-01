#include <gtest/gtest.h>
#include "collections/BitSet.hpp"


using collection::BitSet;


TEST(BitSetTest, ConstructorAndSize)
{
    const BitSet bs(100);
    EXPECT_EQ(bs.size(), 100);
    EXPECT_FALSE(bs.empty());
}


TEST(BitSetTest, EmptyBitSet)
{
    const BitSet bs(0);
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
    EXPECT_TRUE(bs.test(100));
}


TEST(BitSetTest, SizeDataAfterSet)
{
    BitSet bs(64);
    const std::size_t size1 = bs.data().size();
    bs.set(63);
    const std::size_t size2 = bs.data().size();
    EXPECT_EQ(size1, size2);
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

    bs.setAll();

    for (size_t i = 0; i < 100; ++i)
        EXPECT_TRUE(bs.test(i));

    bs.reset();

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

    const BitSet c = a | b;

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

    bs.setAll();

    for (size_t i = 0; i < 70; ++i)
        EXPECT_TRUE(bs.test(i));

    EXPECT_FALSE(bs.test(1000));
}


TEST(BitSetTest, FlipPreservesMask)
{
    BitSet bs(70);

    bs.setAll();
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

    bs.setAll();
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

    const BitSet copy = bs;

    const auto inv = ~bs;

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

TEST(BitSetTest, SetExpandsBitsetSize)
{
    BitSet bs(10);

    bs.set(100);

    EXPECT_TRUE(bs.test(100));
    EXPECT_EQ(bs.size(), 101);
}


TEST(BitSetTest, SetExpandsMultipleBlocks)
{
    BitSet bs(1);

    bs.set(0);
    bs.set(63);
    bs.set(64);
    bs.set(127);
    bs.set(128);
    //
    // EXPECT_TRUE(bs.test(0));
    // EXPECT_TRUE(bs.test(63));
    // EXPECT_TRUE(bs.test(64));
    // EXPECT_TRUE(bs.test(127));
    // EXPECT_TRUE(bs.test(128));
    //
    // EXPECT_EQ(bs.size(), 129);
}


TEST(BitSetTest, SparseGrowthStress)
{
    BitSet bs(0);

    for (size_t i = 0; i < 5000; i += 333)
        bs.set(i);

    for (size_t i = 0; i < 5000; i += 333)
        EXPECT_TRUE(bs.test(i));

    EXPECT_EQ(bs.size(), (5000 / 333) * 333 + 1);
}


TEST(BitSetTest, ResizeDoesNotBreakExistingBits)
{
    BitSet bs(100);

    bs.set(1);
    bs.set(63);

    bs.set(1000);

    EXPECT_TRUE(bs.test(1));
    EXPECT_TRUE(bs.test(63));
    EXPECT_TRUE(bs.test(1000));
}


TEST(BitSetTest, CrossBlockResizeIntegrity)
{
    BitSet bs(10);

    for (size_t i = 0; i < 200; i += 7)
        bs.set(i);

    for (size_t i = 0; i < 200; i += 7)
        EXPECT_TRUE(bs.test(i));

    for (size_t i = 0; i < 200; ++i)
    {
        bool expected = (i % 7 == 0);
        EXPECT_EQ(bs.test(i), expected);
    }
}


TEST(BitSetTest, MassiveIndexGrowth)
{
    BitSet bs(5);

    bs.set(10000);
    bs.set(50000);
    bs.set(100000);

    EXPECT_TRUE(bs.test(10000));
    EXPECT_TRUE(bs.test(50000));
    EXPECT_TRUE(bs.test(100000));

    EXPECT_EQ(bs.size(), 100000 + 1);
}


TEST(BitSetTest, ResizeAndBitwiseORConsistency)
{
    BitSet a(10), b(10);

    a.set(5);
    b.set(200);
    EXPECT_TRUE(a.test(5));
    EXPECT_TRUE(b.test(200));

    const BitSet c = a | b;

    EXPECT_TRUE(c.test(5));
    EXPECT_TRUE(c.test(200));
}


TEST(BitSetTest, DataSizeMatchesInitialSize)
{
    BitSet bs(0);
    EXPECT_EQ(bs.data().size(), 0);

    BitSet bs2(1);
    EXPECT_EQ(bs2.data().size(), 1);

    BitSet bs3(64);
    EXPECT_EQ(bs3.data().size(), 1);

    BitSet bs4(65);
    EXPECT_EQ(bs4.data().size(), 2);
}


TEST(BitSetTest, DataGrowsOnSetBeyondCapacity)
{
    BitSet bs(1);

    bs.set(0);
    EXPECT_EQ(bs.data().size(), 1);

    bs.set(63);
    EXPECT_EQ(bs.data().size(), 1);

    bs.set(64);
    EXPECT_EQ(bs.data().size(), 2);

    bs.set(128);
    EXPECT_EQ(bs.data().size(), 3);
}


TEST(BitSetTest, DataGrowsCorrectlyLargeIndexes)
{
    BitSet bs(0);

    bs.set(0);
    EXPECT_EQ(bs.data().size(), 1);

    bs.set(1000);

    std::size_t expectedBlocks = (1000 + 64) / 64;
    EXPECT_EQ(bs.data().size(), expectedBlocks);

    bs.set(100000);

    expectedBlocks = (100000 + 64) / 64;
    EXPECT_EQ(bs.data().size(), expectedBlocks);
}


TEST(BitSetTest, BasicFlipInvertsBits)
{
    BitSet bs(10);

    bs.set(1);
    bs.set(3);
    bs.set(7);

    bs.flip();

    EXPECT_FALSE(bs.test(1));
    EXPECT_FALSE(bs.test(3));
    EXPECT_FALSE(bs.test(7));

    for (size_t i = 0; i < 10; ++i)
    {
        if (i != 1 && i != 3 && i != 7)
            EXPECT_TRUE(bs.test(i));
    }
}


TEST(BitSetTest, FlipDoesNotExposeGarbageBitsBeyondSize)
{
    BitSet bs(70);

    bs.setAll();
    bs.flip();

    for (size_t i = 0; i < 70; ++i)
        EXPECT_FALSE(bs.test(i));

    EXPECT_FALSE(bs.test(100));
    EXPECT_FALSE(bs.test(200));
}

TEST(BitSetTest, DataDoesNotGrowUnnecessarily)
{
    BitSet bs(200);

    const std::size_t initial = bs.data().size();

    bs.set(10);
    bs.set(50);
    bs.set(199);

    EXPECT_EQ(bs.data().size(), initial);
}


TEST(BitSetTest, DataBlockCountIsExact)
{
    BitSet bs(0);

    for (size_t i = 0; i < 1001; i += 100)
        bs.set(i);

    std::size_t expected = (999 / 64) + 1;

    EXPECT_EQ(bs.data().size(), expected);
}


TEST(BitSetTest, BitwiseOperationsPreserveCorrectSize)
{
    BitSet a(0), b(0);

    a.set(10);
    const size_t sizeA = a.data().size();
    b.set(200);
    const size_t sizeB = b.data().size();

    const BitSet andRes = a & b;
    const BitSet orRes = a | b;
    const BitSet xorRes = a ^ b;

    EXPECT_EQ(andRes.data().size(), sizeA);
    EXPECT_EQ(orRes.data().size(), sizeB);
    EXPECT_EQ(xorRes.data().size(), sizeB);
}


TEST(BitSetTest, ResizeAndBitwiseANDWithEmptyOutsideRange)
{
    BitSet a(10), b(10);

    a.set(5);
    b.set(200);

    BitSet c = a & b;

    EXPECT_FALSE(c.test(5));
    EXPECT_FALSE(c.test(200));
}


TEST(BitSetTest, RandomGrowthPatternStability)
{
    BitSet bs(0);

    std::vector<size_t> positions = {
        1, 2, 5, 8, 64, 65, 128, 129, 1023, 4096, 8192
    };

    for (auto p : positions)
        bs.set(p);

    for (auto p : positions)
        EXPECT_TRUE(bs.test(p));

    EXPECT_EQ(bs.size(), 8192 + 1);
}


TEST(BitSetTest, ResizePreservesLowerBlocks)
{
    BitSet bs(64);

    bs.set(1);
    bs.set(62);

    bs.set(200);

    EXPECT_TRUE(bs.test(1));
    EXPECT_TRUE(bs.test(62));
    EXPECT_TRUE(bs.test(200));
}


TEST(BitSetTest, RepeatedResizeIsStable)
{
    BitSet bs(1);

    for (size_t i = 1; i <= 10; ++i)
        bs.set(i * 1000);

    for (size_t i = 1; i <= 10; ++i)
        EXPECT_TRUE(bs.test(i * 1000));

    EXPECT_EQ(bs.size(), 10000 + 1);
}


TEST(BitSetTest, IteratesSingleBitsCorrectly)
{
    BitSet bs(100);

    bs.set(1);
    bs.set(10);
    bs.set(63);

    std::vector<size_t> result;

    for (auto i : bs)
        result.push_back(i);

    std::vector<size_t> expected = {1, 10, 63};

    EXPECT_EQ(result, expected);
}


TEST(BitSetTest, SkipsUnsetBits)
{
    BitSet bs(100);

    bs.set(0);
    bs.set(50);
    bs.set(99);

    std::vector<size_t> result;

    for (auto i : bs)
        result.push_back(i);

    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(result[0], 0);
    EXPECT_EQ(result[1], 50);
    EXPECT_EQ(result[2], 99);
}


TEST(BitSetTest, IterationIsSorted)
{
    BitSet bs(1000);

    bs.set(900);
    bs.set(10);
    bs.set(500);
    bs.set(1);

    size_t prev = 0;
    bool first = true;

    for (auto i : bs)
    {
        if (!first)
            EXPECT_GT(i, prev);

        prev = i;
        first = false;
    }
}


TEST(BitSetTest, BeginEndConsistency)
{
    BitSet bs(100);

    bs.set(5);
    bs.set(20);

    auto it = bs.begin();
    const auto end = bs.end();

    std::vector<size_t> result;

    while (it != end)
    {
        result.push_back(*it);
        ++it;
    }

    EXPECT_EQ(result, std::vector<size_t>({5, 20}));
}


TEST(BitSetTest, EmptyBitSetIteration)
{
    const BitSet bs(100);

    std::vector<size_t> result;

    for (auto i : bs)
        result.push_back(i);

    EXPECT_TRUE(result.empty());
}


TEST(BitSetTest, SparseLargeValuesIteration)
{
    BitSet bs(0);

    std::vector<size_t> expected;

    for (size_t i = 0; i <= 10000; i += 333)
    {
        bs.set(i);
        expected.push_back(i);
    }

    std::vector<size_t> result;

    for (auto i : bs)
        result.push_back(i);

    EXPECT_EQ(result, expected);
}


TEST(BitSetTest, CrossBlockIteration)
{
    BitSet bs(130);

    bs.set(63);
    bs.set(64);
    bs.set(65);
    bs.set(127);
    bs.set(128);

    std::vector<size_t> result;

    for (auto i : bs)
        result.push_back(i);

    std::vector<size_t> expected = {63, 64, 65, 127, 128};

    EXPECT_EQ(result, expected);
}


TEST(BitSetTest, MultipleIterationsAreStable)
{
    BitSet bs(100);

    bs.set(2);
    bs.set(50);
    bs.set(70);

    std::vector<size_t> first;
    std::vector<size_t> second;

    for (auto i : bs)
        first.push_back(i);

    for (auto i : bs)
        second.push_back(i);

    EXPECT_EQ(first, second);
}


TEST(BitSetTest, IterationAfterResizeSet)
{
    BitSet bs(10);

    bs.set(1);
    bs.set(200);
    bs.set(1000);

    std::vector<size_t> result;

    for (auto i : bs)
        result.push_back(i);

    const std::vector<size_t> expected = {1, 200, 1000};

    EXPECT_EQ(result, expected);
}


TEST(BitSetTest, IsSubsetOfEmptySet)
{
    BitSet empty(0);
    BitSet bs(100);
    bs.set(5);
    bs.set(10);

    EXPECT_TRUE(empty.isSubsetOf(bs));
    EXPECT_TRUE(empty.isSubsetOf(empty));
}


TEST(BitSetTest, IsSubsetOfBigSized)
{
    BitSet bs1(0);
    BitSet bs2(0);
    BitSet bs3(0);
    bs1.set(5);
    bs2.set(10);
    bs3.set(64);

    const std::vector vectorBitSet = {&bs1, &bs2, &bs3};
    for (std::size_t i = 0 ; i < vectorBitSet.size(); ++i)
    {
        for (std::size_t j = 0 ; j < vectorBitSet.size(); ++j)
        {
            if (i != j)
            {
                EXPECT_FALSE(vectorBitSet[i]->isSubsetOf(*vectorBitSet[j]));
            }
        }
    }
}

TEST(BitSetTest, IsSubsetOfEqualSets)
{
    BitSet bs1(100);
    BitSet bs2(100);

    bs1.set(5);
    bs1.set(10);
    bs1.set(42);

    bs2.set(5);
    bs2.set(10);
    bs2.set(42);

    EXPECT_TRUE(bs1.isSubsetOf(bs2));
    EXPECT_TRUE(bs2.isSubsetOf(bs1));
}

TEST(BitSetTest, IsSubsetOfProperSubset)
{
    BitSet superset(100);
    BitSet subset(100);

    superset.set(1);
    superset.set(2);
    superset.set(3);
    superset.set(4);
    superset.set(5);

    subset.set(2);
    subset.set(4);

    EXPECT_TRUE(subset.isSubsetOf(superset));
    EXPECT_FALSE(superset.isSubsetOf(subset));
}

TEST(BitSetTest, IsSubsetOfWithDifferentSizes)
{
    BitSet small(50);
    BitSet large(200);

    small.set(10);
    small.set(30);

    large.set(10);
    large.set(30);
    large.set(100);

    EXPECT_TRUE(small.isSubsetOf(large));

    large.set(250);
    EXPECT_TRUE(small.isSubsetOf(large));
}

TEST(BitSetTest, IsSubsetOfNotSubset)
{
    BitSet bs1(100);
    BitSet bs2(100);

    bs1.set(5);
    bs1.set(10);
    bs1.set(42);

    bs2.set(5);
    bs2.set(10);
    bs2.set(99);

    EXPECT_FALSE(bs1.isSubsetOf(bs2));
    EXPECT_FALSE(bs2.isSubsetOf(bs1));
}

TEST(BitSetTest, IsSubsetOfWithOverlappingBits)
{
    BitSet bs1(100);
    BitSet bs2(100);

    bs1.set(0);
    bs1.set(63);
    bs1.set(64);

    bs2.set(0);
    bs2.set(63);

    EXPECT_FALSE(bs1.isSubsetOf(bs2));
    EXPECT_TRUE(bs2.isSubsetOf(bs1));
}

TEST(BitSetTest, IsSubsetOfEmptyVsNonEmpty)
{
    BitSet empty(0);
    BitSet bs(50);
    bs.set(25);

    EXPECT_TRUE(empty.isSubsetOf(bs));
    EXPECT_FALSE(bs.isSubsetOf(empty));
}

TEST(BitSetTest, MaxOnEmptySet)
{
    BitSet empty(0);
    BitSet empty2(100);

    EXPECT_EQ(empty.max(), static_cast<std::size_t>(-1));
    EXPECT_EQ(empty2.max(), static_cast<std::size_t>(-1));
}

TEST(BitSetTest, MaxWithSingleBit)
{
    BitSet bs(100);

    bs.set(0);
    EXPECT_EQ(bs.max(), 0);

    bs.reset();
    bs.set(42);
    EXPECT_EQ(bs.max(), 42);

    bs.reset();
    bs.set(99);
    EXPECT_EQ(bs.max(), 99);
}

TEST(BitSetTest, MaxWithMultipleBits)
{
    BitSet bs(200);

    bs.set(5);
    bs.set(10);
    bs.set(150);
    bs.set(42);

    EXPECT_EQ(bs.max(), 150);
}

TEST(BitSetTest, MaxOnBlockBoundaries)
{
    BitSet bs(200);

    bs.set(63);
    EXPECT_EQ(bs.max(), 63);

    bs.set(64);
    EXPECT_EQ(bs.max(), 64);

    bs.set(127);
    EXPECT_EQ(bs.max(), 127);

    bs.set(128);
    EXPECT_EQ(bs.max(), 128);
}

TEST(BitSetTest, MaxWithLastBitSet)
{
    BitSet bs(150);
    bs.set(149);
    EXPECT_EQ(bs.max(), 149);
}

TEST(BitSetTest, MaxAfterResize)
{
    BitSet bs(10);
    bs.set(5);
    EXPECT_EQ(bs.max(), 5);

    bs.set(200);
    EXPECT_EQ(bs.max(), 200);

    bs.set(1000);
    EXPECT_EQ(bs.max(), 1000);
}

TEST(BitSetTest, MaxAfterReset)
{
    BitSet bs(200);
    bs.set(50);
    bs.set(100);
    bs.set(150);

    EXPECT_EQ(bs.max(), 150);

    bs.reset(150);
    EXPECT_EQ(bs.max(), 100);

    bs.reset(100);
    EXPECT_EQ(bs.max(), 50);

    bs.reset(50);
    EXPECT_EQ(bs.max(), static_cast<std::size_t>(-1));
}

TEST(BitSetTest, MaxWithAllBitsSet)
{
    BitSet bs(100);
    bs.setAll();

    EXPECT_EQ(bs.max(), 99);
}

TEST(BitSetTest, IsSubsetOfWithMaxMethod)
{
    BitSet bs1(100);
    BitSet bs2(100);

    bs1.set(10);
    bs1.set(20);
    bs1.set(30);

    bs2.set(10);
    bs2.set(20);
    bs2.set(30);
    bs2.set(40);

    if (bs1.isSubsetOf(bs2))
    {
        EXPECT_LE(bs1.max(), bs2.max());
    }

    bs2.set(100);
    EXPECT_TRUE(bs1.isSubsetOf(bs2));
    EXPECT_LE(bs1.max(), bs2.max());
}

TEST(BitSetTest, CombinedSubsetAndMaxOperations)
{
    BitSet base(200);
    base.set(10);
    base.set(50);
    base.set(100);
    base.set(150);

    BitSet subset(200);
    subset.set(50);
    subset.set(100);

    EXPECT_TRUE(subset.isSubsetOf(base));
    EXPECT_EQ(subset.max(), 100);
    EXPECT_EQ(base.max(), 150);

    subset.set(180);
    EXPECT_FALSE(subset.isSubsetOf(base));
    EXPECT_EQ(subset.max(), 180);
}


TEST(BitSetTest, MinWithFirstBitSet)
{
    BitSet bs(150);
    bs.set(0);
    EXPECT_EQ(bs.min(), 0);
}

TEST(BitSetTest, MinWithMiddleBitSet)
{
    BitSet bs(150);
    bs.set(100);
    EXPECT_EQ(bs.min(), 100);
}

TEST(BitSetTest, MinWithMultipleBitsSet)
{
    BitSet bs(150);
    bs.set(50);
    bs.set(75);
    bs.set(100);
    EXPECT_EQ(bs.min(), 50);
}

TEST(BitSetTest, MinWithLastBitSet)
{
    BitSet bs(150);
    bs.set(149);
    EXPECT_EQ(bs.min(), 149);
}

TEST(BitSetTest, MinWithAllBitsSet)
{
    BitSet bs(150);
    for (std::size_t i = 0; i < 150; ++i)
        bs.set(i);
    EXPECT_EQ(bs.min(), 0);
}

TEST(BitSetTest, MinWithEmptySet)
{
    const BitSet bs(150);
    EXPECT_EQ(bs.min(), static_cast<std::size_t>(-1));
}

TEST(BitSetTest, MinWithSingleBitInDifferentBlocks)
{
    BitSet bs(200);

    bs.set(63);
    EXPECT_EQ(bs.min(), 63);
    bs.reset(63);

    bs.set(64);
    EXPECT_EQ(bs.min(), 64);

    bs.set(128);
    EXPECT_EQ(bs.min(), 128);
}

TEST(BitSetTest, LowestBitPositionTest)
{
    EXPECT_EQ(BitSet::lowestBitPosition(0x0000000000000001ULL), 0);
    EXPECT_EQ(BitSet::lowestBitPosition(0x0000000000000002ULL), 1);
    EXPECT_EQ(BitSet::lowestBitPosition(0x0000000000000004ULL), 2);
    EXPECT_EQ(BitSet::lowestBitPosition(0x0000000000000008ULL), 3);
    EXPECT_EQ(BitSet::lowestBitPosition(0x0000000000000010ULL), 4);
    EXPECT_EQ(BitSet::lowestBitPosition(0x0000000080000000ULL), 31);
    EXPECT_EQ(BitSet::lowestBitPosition(0x0000000100000000ULL), 32);
    EXPECT_EQ(BitSet::lowestBitPosition(0x8000000000000000ULL), 63);
}
