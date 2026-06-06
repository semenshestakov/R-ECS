#pragma once


struct TrackedObj
{
    inline static int ctorCount = 0;
    inline static int copyCount = 0;
    inline static int moveCount = 0;

    int value{0};

    TrackedObj(int v = 0) : value(v) { ++ctorCount; }

    TrackedObj(const TrackedObj& other) : value(other.value)
    {
        ++copyCount;
    }

    TrackedObj(TrackedObj&& other) noexcept : value(other.value)
    {
        ++moveCount;
    }

    static void Reset()
    {
        ctorCount = 0;
        copyCount = 0;
        moveCount = 0;
    }
};
