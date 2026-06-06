#ifndef BIT_SET_HPP
#define BIT_SET_HPP
#include <vector>
#include <cstdint>

#include "common_recs/utils/ClassUtils.hpp"


namespace collection
{

    struct BitSet
    {
        struct const_iterator
        {
            using value_type = std::size_t;
            using difference_type = std::ptrdiff_t;
            using reference = std::size_t;
            using pointer = void;
            using iterator_category = std::forward_iterator_tag;

            constexpr const_iterator(const BitSet* bs, std::size_t pos);

            std::size_t operator*() const noexcept;

            const_iterator& operator++();

            [[nodiscard]] bool operator==(const const_iterator& other) const noexcept;
            [[nodiscard]] bool operator!=(const const_iterator& other) const noexcept;

        private:
            const BitSet* m_bs;
            std::size_t m_pos;

            void findNext();
        };

        using dataItem_t = std::uint64_t;
        static constexpr std::size_t BYTE_COUNT = sizeof(dataItem_t);
        static constexpr unsigned char BIT_COUNT = BYTE_COUNT * 8;
        static constexpr std::size_t INVALID_INDEX = static_cast<std::size_t>(-1);

        constexpr BitSet() = default;
        constexpr explicit BitSet(std::size_t size);

    protected:
        using data_t = std::vector<dataItem_t>;
        data_t m_data;
        std::size_t m_size = 0;

    public:
        void set(std::size_t pos) noexcept;
        void reset(std::size_t pos) noexcept;
        void resize(std::size_t pos) noexcept;

        [[nodiscard]] bool test(std::size_t pos) const noexcept;
        [[nodiscard]] bool operator[](std::size_t pos) const noexcept;

        void setAll() noexcept;
        void reset() noexcept;

        [[nodiscard]] const_iterator begin() const noexcept;
        [[nodiscard]] const_iterator end() const noexcept;

        [[nodiscard]] std::size_t max() const;
        [[nodiscard]] std::size_t min() const;
        [[nodiscard]] std::size_t size() const noexcept;
        [[nodiscard]] const data_t& data() const noexcept;
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

        [[nodiscard]] bool isSubsetOf(const BitSet& other) const;

    DEEP_TEST_PRIVATE_ACCESS:
        static constexpr std::size_t highestBitPosition(dataItem_t value);
        static constexpr std::size_t lowestBitPosition(dataItem_t value);
        static constexpr std::size_t getIdx(std::size_t pos) noexcept;
        static constexpr dataItem_t getBit(std::size_t pos) noexcept;
    };

} // namespace collection
#endif
#include "detail/BitSet.ipp"
