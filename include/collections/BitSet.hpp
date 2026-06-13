#ifndef BIT_SET_HPP
#define BIT_SET_HPP
#include <vector>
#include <cstdint>

#include "common_recs/utils/ClassUtils.hpp"


namespace collections
{

    /**
     * @brief Compressed set of integers using a fixed-size bitset.
     * Provides O(1) membership test, iteration over set bits, and bitwise operations.
     * Backed by a vector of 64-bit words.
     */
    struct BitSet
    {
        // ==================================== BitSet::const_iterator ====================================

        /**
         * @brief Forward iterator over set (true) bit positions.
         * Skips zero bits and yields indices of bits that are set.
         */
        struct const_iterator
        {
            using value_type = std::size_t;         ///< Position of a set bit.
            using difference_type = std::ptrdiff_t; ///< Signed distance type.
            using reference = std::size_t;
            using pointer = void;
            using iterator_category = std::forward_iterator_tag; ///< Forward iterator conformance.

            /**
             * @brief Constructs iterator starting search from given position.
             * Advances to the first set bit at or after pos.
             * @param bs Owning BitSet to iterate.
             * @param pos Starting search position.
             */
            constexpr const_iterator(const BitSet* bs, std::size_t pos);

            /**
             * @brief Returns current set bit position.
             * @return Index of current bit.
             */
            std::size_t operator*() const noexcept;

            /**
             * @brief Advances to the next set bit.
             * @return Reference to this iterator.
             */
            const_iterator& operator++();

            /**
             * @brief Checks whether two iterators refer to the same position.
             * @param other Iterator to compare against.
             * @return true if they point to the same position in the same BitSet.
             */
            [[nodiscard]] bool operator==(const const_iterator& other) const noexcept;
            [[nodiscard]] bool operator!=(const const_iterator& other) const noexcept;

        private:
            const BitSet* m_bs = nullptr;   ///< Owning BitSet being iterated.
            std::size_t m_pos = 0;          ///< Current bit position within the BitSet.

            /**
             * @brief Skips forward to the next set bit.
             * Advances m_pos until a set bit is found or end is reached.
             */
            void findNext();
        };

        using dataItem_t = std::uint64_t;                                           ///< Internal word type.
        static constexpr std::size_t BYTE_COUNT = sizeof(dataItem_t);               ///< Bytes per word.
        static constexpr unsigned char BIT_COUNT = BYTE_COUNT * 8;                  ///< Bits per word.
        static constexpr std::size_t INVALID_INDEX = static_cast<std::size_t>(-1);  ///< Sentinel for no set bit.

        /**
         * @brief Constructs empty BitSet with no bits.
         */
        constexpr BitSet() = default;

        /**
         * @brief Constructs BitSet with given capacity, all bits initially zero.
         * @param size Number of bits to allocate.
         */
        constexpr explicit BitSet(std::size_t size);

    protected:
        using data_t = std::vector<dataItem_t>; ///< Internal storage type.
        data_t m_data;                          ///< Raw bit storage.
        std::size_t m_size = 0;                 ///< Bit capacity.

    public:
        /**
         * @brief Sets bit at position to 1. Auto-resizes if needed.
         * @param pos Bit index to set.
         */
        void set(std::size_t pos) noexcept;

        /**
         * @brief Sets bit at position to 0. No-op if pos >= size.
         * @param pos Bit index to clear.
         */
        void reset(std::size_t pos) noexcept;

        /**
         * @brief Resizes storage to accommodate at least pos bits.
         * @param pos Minimum required bit capacity.
         */
        void resize(std::size_t pos) noexcept;

        /**
         * @brief Tests whether bit at position is set.
         * @param pos Bit index to test.
         * @return true if bit is set, false if pos >= size or bit is 0.
         */
        [[nodiscard]] bool test(std::size_t pos) const noexcept;

        /**
         * @brief Alias for test(pos).
         * @param pos Bit index to test.
         * @return true if bit is set.
         */
        [[nodiscard]] bool operator[](std::size_t pos) const noexcept;

        /**
         * @brief Sets all bits to 1 within current capacity.
         * Spare bits in the last word are masked out.
         */
        void setAll() noexcept;

        /**
         * @brief Clears all bits to 0.
         */
        void reset() noexcept;

        /**
         * @brief Returns iterator to the first set bit.
         * @return Iterator positioned at the first set bit, or end() if none.
         */
        [[nodiscard]] const_iterator begin() const noexcept;

        /**
         * @brief Returns past-the-end iterator.
         * @return Iterator with position equal to size().
         */
        [[nodiscard]] const_iterator end() const noexcept;

        /**
         * @brief Returns index of the highest set bit.
         * @return Bit index, or INVALID_INDEX if empty.
         */
        [[nodiscard]] std::size_t max() const;

        /**
         * @brief Returns index of the lowest set bit.
         * @return Bit index, or INVALID_INDEX if empty.
         */
        [[nodiscard]] std::size_t min() const;

        /**
         * @brief Returns current bit capacity.
         * @return Number of bits allocated.
         */
        [[nodiscard]] std::size_t size() const noexcept;

        /**
         * @brief Returns const reference to internal word storage.
         * @return Underlying vector of 64-bit words.
         */
        [[nodiscard]] const data_t& data() const noexcept;

        /**
         * @brief Checks whether the BitSet has zero capacity.
         * @return true if size() == 0.
         */
        [[nodiscard]] bool empty() const noexcept;

        /**
         * @brief Inverts all bits in-place.
         * Spare bits in the last word are masked out.
         * @return Reference to this BitSet.
         */
        [[maybe_unused]] BitSet& flip() noexcept;

        /**
         * @brief Bitwise AND of two BitSets.
         * @param other Right-hand operand.
         * @return New BitSet containing result.
         */
        [[nodiscard]] BitSet operator&(const BitSet& other) const;

        /**
         * @brief Bitwise OR of two BitSets.
         * Auto-resizes to the larger size.
         * @param other Right-hand operand.
         * @return New BitSet containing result.
         */
        [[nodiscard]] BitSet operator|(const BitSet& other) const;

        /**
         * @brief Bitwise XOR of two BitSets.
         * Auto-resizes to the larger size.
         * @param other Right-hand operand.
         * @return New BitSet containing result.
         */
        [[nodiscard]] BitSet operator^(const BitSet& other) const;

        /**
         * @brief Compound bitwise AND assignment.
         * @param other Right-hand operand.
         * @return Reference to this BitSet.
         */
        BitSet& operator&=(const BitSet& other);

        /**
         * @brief Compound bitwise OR assignment.
         * Auto-resizes to fit other.
         * @param other Right-hand operand.
         * @return Reference to this BitSet.
         */
        BitSet& operator|=(const BitSet& other);

        /**
         * @brief Compound bitwise XOR assignment.
         * Auto-resizes to fit other.
         * @param other Right-hand operand.
         * @return Reference to this BitSet.
         */
        BitSet& operator^=(const BitSet& other);

        /**
         * @brief Returns a copy with all bits inverted.
         * @return New BitSet containing flipped result.
         */
        [[nodiscard]] BitSet operator~() const;

        /**
         * @brief Checks whether two BitSets have the same size and bits.
         * @param other BitSet to compare against.
         * @return true if identical.
         */
        [[nodiscard]] bool operator==(const BitSet& other) const;

        /**
         * @brief Checks whether two BitSets differ.
         * @param other BitSet to compare against.
         * @return true if not identical.
         */
        [[nodiscard]] bool operator!=(const BitSet& other) const;

        /**
         * @brief Checks whether all set bits of this are also set in other.
         * @param other BitSet to check against.
         * @return true if this is a subset of other.
         */
        [[nodiscard]] bool isSubsetOf(const BitSet& other) const;

    DEEP_TEST_PRIVATE_ACCESS:
        /**
         * @brief Finds the index of the highest set bit in a word.
         * @param value 64-bit word to scan.
         * @return Bit position (0-63).
         */
        static constexpr std::size_t highestBitPosition(dataItem_t value);

        /**
         * @brief Finds the index of the lowest set bit in a word.
         * @param value 64-bit word to scan.
         * @return Bit position (0-63).
         */
        static constexpr std::size_t lowestBitPosition(dataItem_t value);

        /**
         * @brief Converts bit position to word index (divide by 64).
         * @param pos Bit index.
         * @return Word index in m_data.
         */
        static constexpr std::size_t getIdx(std::size_t pos) noexcept;

        /**
         * @brief Creates a mask with a single bit at given position within a word.
         * @param pos Bit index.
         * @return 64-bit mask with one bit set.
         */
        static constexpr dataItem_t getBit(std::size_t pos) noexcept;
    };

} // namespace collections
#endif
#include "detail/BitSet.ipp"
