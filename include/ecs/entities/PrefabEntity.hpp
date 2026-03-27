#pragma once
#include <vector>
#include "ecs/utils/ComponentUtils.hpp"


namespace ecs
{

    struct PrefabEntity
    {
        PrefabEntity() = default;

        template<DerivedComponent ComponentCls, typename... Args>
        void AddComponent(Args&&... args);

        using componentsData_t = std::vector<std::unique_ptr<byte[]>>;
        [[nodiscard]] const componentsData_t& GetComponentsData() const { return m_dataByComponentsIndex; }

        void clear();

    private:
        componentsData_t m_dataByComponentsIndex {};
    };


    template<DerivedComponent ComponentCls, typename... Args>
    void PrefabEntity::AddComponent(Args&&... args)
    {
        if(m_dataByComponentsIndex.size() < ComponentCls::componentId)
        {
            m_dataByComponentsIndex.resize(ComponentCls::componentId + 1);
        }

        auto ptr = std::make_unique<byte[]>(sizeof(ComponentCls));
        new(ptr.get()) ComponentCls(std::forward<Args>(args)...);

        m_dataByComponentsIndex[ComponentCls::componentId] = std::move(ptr);
    }

    inline void PrefabEntity::clear() { m_dataByComponentsIndex.clear(); }

} // namespace ecs
