#ifndef ENTITIES_ARCHETYPE_STORAGE_HPP
#define ENTITIES_ARCHETYPE_STORAGE_HPP

#include <vector>
#include <queue>
#include "ecs/utils/ComponentUtils.hpp"
#include "ecs/utils/EntitiesUtils.hpp"


namespace ecs
{

    struct PrefabEntity;
    struct Entity;

    using chunkEntityIndex_t = std::uint32_t;
    constexpr chunkEntityIndex_t MAX_ENTITIES_IN_CHUNK = 256;
    constexpr chunkEntityIndex_t MAX_ENTITIES_IN_CHUNK_MASK = MAX_ENTITIES_IN_CHUNK - 1;
    constexpr chunkEntityIndex_t MAX_ENTITIES_IN_CHUNKS = ~0u;

    // ========================================= ArchetypedChunkEntityLocation =========================================

    using archetypeHash_t = std::size_t;
    using archetypeIndex_t = std::uint16_t;

    struct ArchetypedChunkEntityLocation final
    {
        archetypeIndex_t archetypeIndex;
        chunkEntityIndex_t chunkEntityIndex;

        [[nodiscard]] bool operator==(const ArchetypedChunkEntityLocation& other) const;
        [[nodiscard]] bool operator!=(const ArchetypedChunkEntityLocation& other) const;
    };

    struct Archetype final
    {
        archetypeHash_t hash {};
        std::vector<componentId_t> componentsIds;
        std::vector<bool> mask;

        static constexpr archetypeHash_t GetArchetypeHash(const componentId_t* begin, const componentId_t* end);
        template<DerivedComponent... ComponentCls> static const Archetype& GetArchetype();
        static std::vector<bool> GetArchetypeMask(const componentId_t* begin, const componentId_t* end);

        template<DerivedComponent... ComponentCls> [[nodiscard]] bool matchArchetype() const;
    };

    // =============================================== ArchetypedChunks ===============================================

    class ArchetypedChunks final
    {
    public:
        ArchetypedChunks() = delete;
        explicit ArchetypedChunks(Archetype&& a_archetype);

        ArchetypedChunks(const ArchetypedChunks& other) = delete;
        ArchetypedChunks& operator=(const ArchetypedChunks& other) = delete;

        ArchetypedChunks(ArchetypedChunks&&) noexcept = default;
        ArchetypedChunks& operator=(ArchetypedChunks&&) noexcept = default;

        chunkEntityIndex_t Create(const PrefabEntity& entity);
        void Destroy(chunkEntityIndex_t chunkEntityIndex);
        [[nodiscard]] bool IsAlive(chunkEntityIndex_t chunkEntityIndex) const;

        [[nodiscard]] byte* GetComponentData(chunkEntityIndex_t chunkEntityIndex, componentId_t componentId);
        [[nodiscard]] const byte* GetComponentData(chunkEntityIndex_t chunkEntityIndex, componentId_t componentId) const;

        [[nodiscard]] chunkEntityIndex_t getLastChunkEntityIndex() const;
        [[nodiscard]] static std::size_t getChunkByEntityIndex(chunkEntityIndex_t chunkEntityIndex);
        [[nodiscard]] static std::size_t getLocalEntityIndex(chunkEntityIndex_t chunkEntityIndex);

        [[nodiscard]] const Archetype& archetype() const;

    private:
        Archetype m_archetype;
        using componentChunks_t = std::vector<std::unique_ptr<byte[]>>;
        std::vector<componentChunks_t> m_chunksByComponentId {};

        std::vector<bool> m_isInWorld;
        chunkEntityIndex_t m_lastChunkEntityIndex = 0;
        std::queue<chunkEntityIndex_t> m_freeChunkEntityIndex;
    };

    // =========================================== EntitiesArchetypeStorage ===========================================

    class EntitiesArchetypeStorage final
    {
    public:
        // ==================================== EntitiesArchetypeStorage::iterator ====================================
        template<typename ValueType, DerivedComponent... ComponentCls>
        class iterator
        {
        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = ValueType;
            using difference_type = std::ptrdiff_t;
            using pointer = const value_type*;
            using reference = const value_type&;

            iterator() = default;
            explicit iterator(EntitiesArchetypeStorage* storage, bool isEnd = false);

            reference operator*() const;
            pointer operator->() const;

            iterator& operator++();
            iterator operator++(int);

            bool operator==(const iterator& other) const;
            bool operator!=(const iterator& other) const;

            iterator begin() const;
            iterator end() const;

        private:
            void advance();
            void advanceToNextValid();

            EntitiesArchetypeStorage* m_storage = nullptr;
            archetypeIndex_t m_currentArchetype = 0;
            ArchetypedChunkEntityLocation m_entityLocation {};

            bool m_isEnded = false;
        };

        template<DerivedComponent... ComponentCls>
        using iter_value_type = std::conditional_t<
            (sizeof...(ComponentCls) > 0),
            std::tuple<ComponentCls&...>,
            ArchetypedChunkEntityLocation
        >;

        template<DerivedComponent... ComponentCls>
        [[nodiscard]] auto begin();

        template<DerivedComponent... ComponentCls>
        [[nodiscard]] auto end();

        // ========================================= EntitiesArchetypeStorage =========================================

        ArchetypedChunkEntityLocation Create(const PrefabEntity& prefabEntity);
        void Destroy(const ArchetypedChunkEntityLocation& entityLocation);
        [[nodiscard]] bool IsAlive(const ArchetypedChunkEntityLocation& entityLocation) const;

        [[nodiscard]] byte* GetComponentData(const ArchetypedChunkEntityLocation& entityLocation, componentId_t componentId);
        [[nodiscard]] const byte* GetComponentData(const ArchetypedChunkEntityLocation& entityLocation, componentId_t componentId) const;

        template<DerivedComponent ComponentCls>
        [[nodiscard]] ComponentCls* TryGetComponent(const ArchetypedChunkEntityLocation& location);

        template<DerivedComponent ComponentCls>
        [[nodiscard]] const ComponentCls* TryGetComponent(const ArchetypedChunkEntityLocation& location) const;

        template<DerivedComponent ComponentCls>
        [[nodiscard]] ComponentCls& GetComponent(const ArchetypedChunkEntityLocation& location);

        template<DerivedComponent ComponentCls>
        [[nodiscard]] const ComponentCls& GetComponent(const ArchetypedChunkEntityLocation& location) const;

    private:
        std::unordered_map<archetypeHash_t, archetypeIndex_t> m_archetypeIndexByHash;
        std::vector<ArchetypedChunks> m_storageByArchetypeIndex;
    };

} // namespace ecs
#endif
#include "detail/EntitiesArchetypeStorage.ipp"
