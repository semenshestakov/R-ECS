#ifndef BIT_SET_HPP
#define BIT_SET_HPP
#include <vector>
#include <cstdint>


namespace collection
{

    struct BitSet
    {
        using dataItem_t = std::uint64_t;
        static constexpr std::size_t BYTE_COUNT = sizeof(dataItem_t);
        static constexpr unsigned char BIT_COUNT = BYTE_COUNT * 8;

        explicit BitSet(std::size_t size);

        void set(std::size_t pos) noexcept;
        void reset(std::size_t pos) noexcept;
        [[nodiscard]] bool test(std::size_t pos) const noexcept;
        [[nodiscard]] bool operator[](std::size_t pos) noexcept;
        [[nodiscard]] bool operator[](std::size_t pos) const noexcept;

        void set_all() noexcept;
        void reset_all() noexcept;

        [[nodiscard]] std::size_t size() const noexcept;
        [[nodiscard]] bool empty() const noexcept;

        [[maybe_unused]] BitSet& flip() noexcept;

        [[nodiscard]] BitSet operator&(const BitSet& other) const;
        [[nodiscard]] BitSet operator|(const BitSet& other) const;
        [[nodiscard]] BitSet operator^(const BitSet& other) const;

        BitSet& operator&=(const BitSet& other);
        BitSet& operator|=(const BitSet& other);
        BitSet& operator^=(const BitSet& other);
        [[nodiscard]] BitSet operator~() const;

        [[nodiscard]] bool operator==(const BitSet& other) const;
        [[nodiscard]] bool operator!=(const BitSet& other) const;

    private:
        static constexpr std::size_t getIdx(std::size_t pos) noexcept;
        static constexpr dataItem_t getBit(std::size_t pos) noexcept;

        using data_t = std::vector<dataItem_t>;
        data_t m_data;
        std::size_t m_size;
    };

} // namespace collection
#endif
#include "detail/BitSet.ipp"
