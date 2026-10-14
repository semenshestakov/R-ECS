#pragma once
#include <algorithm>

#include "../BitSet.hpp"

// = = = = = = = = = = = = = = = = = = = = = = = = = = const_iterator = = = = = = = = = = = = = = = = = = = = = = = = = =

constexpr collections::BitSet::const_iterator::const_iterator(const BitSet* bs, const std::size_t pos) :
    m_bs(bs),
    m_pos(pos)
{
    findNext();
}

constexpr std::size_t collections::BitSet::const_iterator::operator*() const noexcept
{
    return m_pos;
}

constexpr collections::BitSet::const_iterator& collections::BitSet::const_iterator::operator++()
{
    ++m_pos;
    findNext();
    return *this;
}

constexpr bool collections::BitSet::const_iterator::operator==(const const_iterator& other) const noexcept
{
    return m_pos == other.m_pos && m_bs == other.m_bs;
}

constexpr bool collections::BitSet::const_iterator::operator!=(const const_iterator& other) const noexcept
{
    return !(*this == other);
}

constexpr void collections::BitSet::const_iterator::findNext()
{
    while (m_pos < m_bs->size() && !m_bs->test(m_pos))
    {
        ++m_pos;
    }
}

constexpr collections::BitSet::const_iterator collections::BitSet::begin() const noexcept
{
    return {this, 0};
}

constexpr collections::BitSet::const_iterator collections::BitSet::end() const noexcept
{
    return {this, m_size};
}

// = = = = = = = = = = = = = = = = = = = = = = = = = = = = BitSet = = = = = = = = = = = = = = = = = = = = = = = = = = =


constexpr collections::BitSet::BitSet(const std::size_t size) :
    m_data((size + BIT_COUNT - 1) / BIT_COUNT, 0),
    m_size(size)
{
}

constexpr void collections::BitSet::set(const std::size_t pos) noexcept
{
    if (pos >= m_size)
        resize(pos + 1);
    m_data[getIdx(pos)] |= getBit(pos);
}

constexpr void collections::BitSet::reset(const std::size_t pos) noexcept
{
    if (pos >= m_size)
        return;
    m_data[getIdx(pos)] &= ~getBit(pos);
}

constexpr void collections::BitSet::resize(const std::size_t pos) noexcept
{
    const auto oldSize = m_size;
    m_size = pos;
    const std::size_t requiredBlocks = (m_size + BIT_COUNT - 1) / BIT_COUNT;

    if (m_data.size() < requiredBlocks)
    {
        m_data.resize(requiredBlocks);
    }

    if (m_size < oldSize)
    {
        const std::size_t oldRequiredBlocks = (oldSize + BIT_COUNT - 1) / BIT_COUNT;

        if (requiredBlocks < oldRequiredBlocks)
            std::fill(m_data.begin() + requiredBlocks, m_data.begin() + oldRequiredBlocks, dataItem_t{0});

        if (const auto lastBits = m_size & 63)
            m_data.back() &= (static_cast<dataItem_t>(1) << lastBits) - 1;
    }
}

constexpr bool collections::BitSet::test(const std::size_t pos) const noexcept
{
    if (pos >= m_size)
        return false;

    return (m_data[getIdx(pos)] & getBit(pos)) != 0;
}

constexpr bool collections::BitSet::operator[](const std::size_t pos) const noexcept
{
    return test(pos);
}

constexpr void collections::BitSet::setAll() noexcept
{
    std::ranges::fill(m_data, ~dataItem_t{0});

    if (const auto last_bits = m_size & 63)
        m_data.back() &= (static_cast<dataItem_t>(1) << last_bits) - 1;
}

constexpr void collections::BitSet::reset() noexcept
{
    std::ranges::fill(m_data, 0);
}

constexpr std::size_t collections::BitSet::max() const
{
    if (empty())
        return INVALID_INDEX;

    for (std::size_t i = m_data.size(); i > 0; --i)
    {
        const std::size_t blockIdx = i - 1;
        if (const dataItem_t block = m_data[blockIdx]; block != 0)
        {
            const std::size_t bitPos = highestBitPosition(block);
            return (blockIdx * BIT_COUNT) + bitPos;
        }
    }

    return INVALID_INDEX;
}

constexpr std::size_t collections::BitSet::min() const
{
    if (empty())
        return INVALID_INDEX;

    for (std::size_t i = 0; i < m_data.size(); ++i)
    {
        if (const dataItem_t block = m_data[i]; block != 0)
        {
            const std::size_t bitPos = lowestBitPosition(block);
            return (i * BIT_COUNT) + bitPos;
        }
    }

    return INVALID_INDEX;
}

constexpr std::size_t collections::BitSet::size() const noexcept
{
    return m_size;
}

constexpr const collections::BitSet::data_t& collections::BitSet::data() const noexcept
{
    return m_data;
}

constexpr bool collections::BitSet::empty() const noexcept
{
    return m_size == 0;
}

constexpr collections::BitSet& collections::BitSet::flip() noexcept
{
    for (auto& word : m_data)
        word = ~word;

    if (const auto last_bits = m_size & 63)
        m_data.back() &= (static_cast<dataItem_t>(1) << last_bits) - 1;

    return *this;
}

constexpr collections::BitSet collections::BitSet::operator&(const BitSet& other) const
{
    BitSet result = *this;
    result &= other;
    return result;
}

constexpr collections::BitSet collections::BitSet::operator|(const BitSet& other) const
{
    BitSet result = *this;
    result |= other;
    return result;
}

constexpr collections::BitSet collections::BitSet::operator^(const BitSet& other) const
{
    BitSet result = *this;
    result ^= other;
    return result;
}

constexpr collections::BitSet& collections::BitSet::operator&=(const BitSet& other)
{
    const auto sharedWords = std::min(m_data.size(), other.m_data.size());

    for (std::size_t i = 0; i < sharedWords; ++i)
        m_data[i] &= other.m_data[i];

    for (std::size_t i = sharedWords; i < m_data.size(); ++i)
        m_data[i] = 0;

    if (m_size > other.m_size)
        m_size = other.m_size;

    return *this;
}

constexpr collections::BitSet& collections::BitSet::operator|=(const BitSet& other)
{
    if (m_size < other.m_size)
        resize(other.m_size);

    for (std::size_t i = 0; i < m_data.size(); ++i)
        m_data[i] |= other.m_data[i];

    return *this;
}

constexpr collections::BitSet& collections::BitSet::operator^=(const BitSet& other)
{
    if (m_size < other.m_size)
        resize(other.m_size);

    for (std::size_t i = 0; i < m_data.size(); ++i)
        m_data[i] ^= other.m_data[i];

    if (const auto last_bits = m_size & 63)
        m_data.back() &= (static_cast<dataItem_t>(1) << last_bits) - 1;

    return *this;
}

constexpr collections::BitSet collections::BitSet::operator~() const
{
    BitSet result = *this;
    result.flip();
    return result;
}

constexpr bool collections::BitSet::operator==(const BitSet& other) const
{
    if (m_size != other.m_size)
        return false;
    return m_data == other.m_data;
}

constexpr bool collections::BitSet::operator!=(const BitSet& other) const
{
    return !(*this == other);
}

constexpr bool collections::BitSet::isSubsetOf(const BitSet& other) const
{
    if (m_size == 0)
        return true;

    for (std::size_t i = 0; i < m_data.size(); ++i)
    {
        if (other.m_data.size() <= i)
            return false;

        if ((m_data[i] & ~other.m_data[i]) != 0)
            return false;
    }

    return true;
}

/* static */ constexpr std::size_t collections::BitSet::highestBitPosition(dataItem_t value)
{
    std::size_t pos = 0;

    if (value & 0xFFFFFFFF00000000ULL) { pos += 32; value >>= 32; }
    if (value & 0x00000000FFFF0000ULL) { pos += 16; value >>= 16; }
    if (value & 0x000000000000FF00ULL) { pos += 8;  value >>= 8;  }
    if (value & 0x00000000000000F0ULL) { pos += 4;  value >>= 4;  }
    if (value & 0x000000000000000CULL) { pos += 2;  value >>= 2;  }
    if (value & 0x0000000000000002ULL) { pos += 1;                }

    return pos;
}

/* static */ constexpr std::size_t collections::BitSet::lowestBitPosition(dataItem_t value)
{
    std::size_t pos = 0;

    if ((value & 0x00000000FFFFFFFFULL) == 0) { pos += 32; value >>= 32; }
    if ((value & 0x000000000000FFFFULL) == 0) { pos += 16; value >>= 16; }
    if ((value & 0x00000000000000FFULL) == 0) { pos += 8;  value >>= 8;  }
    if ((value & 0x000000000000000FULL) == 0) { pos += 4;  value >>= 4;  }
    if ((value & 0x0000000000000003ULL) == 0) { pos += 2;  value >>= 2;  }
    if ((value & 0x0000000000000001ULL) == 0) { pos += 1;                }

    return pos;
}

/* static */ constexpr std::size_t collections::BitSet::getIdx(const std::size_t pos) noexcept
{
    return pos >> 6;  // pos / 64 - faster than division
}

/* static */ constexpr collections::BitSet::dataItem_t collections::BitSet::getBit(const std::size_t pos) noexcept
{
    return static_cast<dataItem_t>(1) << (pos & 63); // pos % 64
}
