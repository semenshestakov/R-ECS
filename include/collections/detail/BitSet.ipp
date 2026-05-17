#pragma once
#include <ranges>
#include "../BitSet.hpp"

// = = = = = = = = = = = = = = = = = = = = = = = = = = const_iterator = = = = = = = = = = = = = = = = = = = = = = = = = =

inline collection::BitSet::const_iterator::const_iterator(const BitSet* bs, const std::size_t pos) :
    m_bs(bs),
    m_pos(pos)
{
    findNext();
}

inline std::size_t collection::BitSet::const_iterator::operator*() const noexcept
{
    return m_pos;
}

inline collection::BitSet::const_iterator& collection::BitSet::const_iterator::operator++()
{
    ++m_pos;
    findNext();
    return *this;
}

inline bool collection::BitSet::const_iterator::operator==(const const_iterator& other) const noexcept
{
    return m_pos == other.m_pos && m_bs == other.m_bs;
}

inline bool collection::BitSet::const_iterator::operator!=(const const_iterator& other) const noexcept
{
    return !(*this == other);
}

inline void collection::BitSet::const_iterator::findNext()
{
    while (m_pos < m_bs->size() && !m_bs->test(m_pos))
    {
        ++m_pos;
    }
}

inline collection::BitSet::const_iterator collection::BitSet::begin() const noexcept
{
    return {this, 0};
}

inline collection::BitSet::const_iterator collection::BitSet::end() const noexcept
{
    return {this, m_size};
}

// = = = = = = = = = = = = = = = = = = = = = = = = = = = = BitSet = = = = = = = = = = = = = = = = = = = = = = = = = = = =

inline collection::BitSet::BitSet(const std::size_t size) :
    m_data((size + BIT_COUNT - 1) / BIT_COUNT, 0),
    m_size(size)
{
}

inline void collection::BitSet::set(const std::size_t pos) noexcept
{
    resize(pos);
    m_data[getIdx(pos)] |= getBit(pos);
}

inline void collection::BitSet::reset(const std::size_t pos) noexcept
{
    if (pos >= m_size)
        return;
    m_data[getIdx(pos)] &= ~getBit(pos);
}

inline void collection::BitSet::resize(const std::size_t pos) noexcept
{
    if (const std::size_t newSize = pos + 1; newSize > m_size)
    {
        m_size = newSize;

        if (const std::size_t requiredBlocks = (m_size + BIT_COUNT - 1) / BIT_COUNT; m_data.size() < requiredBlocks)
        {
            m_data.resize(requiredBlocks, 0);
        }
    }
}

inline bool collection::BitSet::test(const std::size_t pos) const noexcept
{
    return (m_data[getIdx(pos)] & getBit(pos)) != 0;
}

inline bool collection::BitSet::operator[](const std::size_t pos) const noexcept
{
    return test(pos);
}

inline void collection::BitSet::setAll() noexcept
{
    std::ranges::fill(m_data, ~dataItem_t{0});

    if (const auto last_bits = m_size & 63)
        m_data.back() &= (static_cast<dataItem_t>(1) << last_bits) - 1;
}

inline void collection::BitSet::reset() noexcept
{
    std::ranges::fill(m_data, 0);
}

inline std::size_t collection::BitSet::size() const noexcept
{
    return m_size;
}

inline const collection::BitSet::data_t& collection::BitSet::data() const noexcept
{
    return m_data;
}

inline bool collection::BitSet::empty() const noexcept
{
    return m_size == 0;
}

inline collection::BitSet& collection::BitSet::flip() noexcept
{
    for (auto& word : m_data)
        word = ~word;

    if (const auto last_bits = m_size & 63)
        m_data.back() &= (static_cast<dataItem_t>(1) << last_bits) - 1;

    return *this;
}

inline collection::BitSet collection::BitSet::operator&(const BitSet& other) const
{
    BitSet result = *this;
    result &= other;
    return result;
}

inline collection::BitSet collection::BitSet::operator|(const BitSet& other) const
{
    BitSet result = *this;
    result |= other;
    return result;
}

inline collection::BitSet collection::BitSet::operator^(const BitSet& other) const
{
    BitSet result = *this;
    result ^= other;
    return result;
}

inline collection::BitSet& collection::BitSet::operator&=(const BitSet& other)
{
    for (std::size_t i = 0; i < m_data.size(); ++i)
        m_data[i] &= other.m_data[i];

    return *this;
}

inline collection::BitSet& collection::BitSet::operator|=(const BitSet& other)
{
    resize(other.m_size);
    for (std::size_t i = 0; i < m_data.size(); ++i)
        m_data[i] |= other.m_data[i];

    return *this;
}

inline collection::BitSet& collection::BitSet::operator^=(const BitSet& other)
{
    resize(other.m_size);
    for (std::size_t i = 0; i < m_data.size(); ++i)
        m_data[i] ^= other.m_data[i];

    if (const auto last_bits = m_size & 63)
        m_data.back() &= (static_cast<dataItem_t>(1) << last_bits) - 1;

    return *this;
}

inline collection::BitSet collection::BitSet::operator~() const
{
    BitSet result = *this;
    result.flip();
    return result;
}

inline bool collection::BitSet::operator==(const BitSet& other) const
{
    if (m_size != other.m_size)
        return false;
    return m_data == other.m_data;
}

inline bool collection::BitSet::operator!=(const BitSet& other) const
{
    return !(*this == other);
}

/* static */ constexpr std::size_t collection::BitSet::getIdx(const std::size_t pos) noexcept
{
    return pos >> 6;  // pos / 64 - faster than division
}

/* static */ constexpr collection::BitSet::dataItem_t collection::BitSet::getBit(std::size_t pos) noexcept
{
    return static_cast<dataItem_t>(1) << (pos & 63); // pos % 64
}
