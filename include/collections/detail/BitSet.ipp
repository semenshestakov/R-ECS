#pragma once
#include <ranges>
#include "../BitSet.hpp"


inline collection::BitSet::BitSet(const std::size_t size) :
    m_data((size + BIT_COUNT - 1) / BIT_COUNT, 0),
    m_size(size)
{
}

inline void collection::BitSet::set(const std::size_t pos) noexcept
{
    if (pos >= m_size)
        return;
    m_data[getIdx(pos)] |= getBit(pos);
}

inline void collection::BitSet::reset(const std::size_t pos) noexcept
{
    if (pos >= m_size)
        return;
    m_data[getIdx(pos)] &= ~getBit(pos);
}

inline bool collection::BitSet::test(const std::size_t pos) const noexcept
{
    if (pos >= m_size)
        return false;

    return (m_data[getIdx(pos)] & getBit(pos)) != 0;
}

inline bool collection::BitSet::operator[](const std::size_t pos) noexcept
{
    return test(pos);
}

inline bool collection::BitSet::operator[](const std::size_t pos) const noexcept
{
    return test(pos);
}

inline void collection::BitSet::set_all() noexcept
{
    std::ranges::fill(m_data, ~dataItem_t{0});

    if (const auto last_bits = m_size & 63)
        m_data.back() &= (static_cast<dataItem_t>(1) << last_bits) - 1;
}

inline void collection::BitSet::reset_all() noexcept
{
    std::ranges::fill(m_data, 0);
}

inline std::size_t collection::BitSet::size() const noexcept
{
    return m_size;
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
    for (std::size_t i = 0; i < m_data.size(); ++i)
        m_data[i] |= other.m_data[i];

    return *this;
}

inline collection::BitSet& collection::BitSet::operator^=(const BitSet& other)
{
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
