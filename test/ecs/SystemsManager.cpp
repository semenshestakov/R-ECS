#include "ecs/systems/SystemsManager.hpp"
#include "ecs/registry/RegistryRegistrator.hpp"
#include "SystemsClass.hpp"
#include "gtest/gtest.h"


using namespace ecs;


TEST(SystemsManagerSizeTest, EmptyManager_SizeIsZero)
{
    const SystemsManager mgr;
    EXPECT_EQ(mgr.size(), 0);
}


TEST(SystemsManagerSizeTest, SizeMatchesRegisteredCount)
{
    const auto mgr = SystemsManager::Create(RegistryRegistrator::Get("test1").systemRegIndexes);
    EXPECT_EQ(mgr.size(), 2);
}


TEST(SystemsManagerSizeTest, SizeAfterManualRegister)
{
    SystemsManager mgr;
    mgr.Register<AutoSystem1>();
    EXPECT_EQ(mgr.size(), 1);
    mgr.Register<AutoSystem2>();
    EXPECT_EQ(mgr.size(), 2);
}


TEST(SystemsManagerRegisterTest, Register_ReturnsTrueFirstTime)
{
    SystemsManager mgr;
    EXPECT_TRUE(mgr.Register<AutoSystem1>());
}


TEST(SystemsManagerRegisterTest, Register_ReturnsFalseForDuplicate)
{
    SystemsManager mgr;
    mgr.Register<AutoSystem1>();
    EXPECT_FALSE(mgr.Register<AutoSystem1>());
}


TEST(SystemsManagerRegisterTest, DuplicateRegister_DoesNotIncrementSize)
{
    SystemsManager mgr;
    mgr.Register<AutoSystem1>();
    mgr.Register<AutoSystem1>();
    EXPECT_EQ(mgr.size(), 1);
}


TEST(SystemsManagerTryGetTest, TryGet_ExistingSystem_ReturnsNonNull)
{
    const auto mgr = SystemsManager::Create(RegistryRegistrator::Get("test1").systemRegIndexes);
    EXPECT_NE(mgr.TryGet<AutoSystem1>(), nullptr);
    EXPECT_NE(mgr.TryGet<AutoSystem2>(), nullptr);
}

TEST(SystemsManagerTryGetTest, TryGet_MissingSystem_ReturnsNull)
{
    const auto mgr = SystemsManager::Create(RegistryRegistrator::Get("test1").systemRegIndexes);
    EXPECT_EQ(mgr.TryGet<SystemTestUpdate>(), nullptr);
}

TEST(SystemsManagerTryGetTest, TryGet_EmptyManager_ReturnsNull)
{
    SystemsManager mgr;
    EXPECT_EQ(mgr.TryGet<AutoSystem1>(), nullptr);
}

TEST(SystemsManagerTryGetTest, TryGet_AfterManualRegister_ReturnsNonNull)
{
    SystemsManager mgr;
    mgr.Register<AutoSystem1>();
    EXPECT_NE(mgr.TryGet<AutoSystem1>(), nullptr);
}


TEST(SystemsManagerGetTest, Get_ReturnsSamePointerAsTryGet)
{
    const auto mgr = SystemsManager::Create(RegistryRegistrator::Get("test1").systemRegIndexes);

    auto& ref  = mgr.Get<AutoSystem1>();
    auto* ptr  = mgr.TryGet<AutoSystem1>();
    EXPECT_EQ(&ref, ptr);
}


TEST(SystemsManagerMoveTest, MoveConstructor_TransfersSystems)
{
    auto mgr1 = SystemsManager::Create(RegistryRegistrator::Get("test1").systemRegIndexes);
    ASSERT_EQ(mgr1.size(), 2);

    SystemsManager mgr2(std::move(mgr1));

    EXPECT_EQ(mgr2.size(), 2);
    EXPECT_NE(mgr2.TryGet<AutoSystem1>(), nullptr);
    EXPECT_NE(mgr2.TryGet<AutoSystem2>(), nullptr);
}

TEST(SystemsManagerMoveTest, MoveAssignment_TransfersSystems)
{
    auto mgr1 = SystemsManager::Create(RegistryRegistrator::Get("test1").systemRegIndexes);
    SystemsManager mgr2;

    mgr2 = std::move(mgr1);

    EXPECT_EQ(mgr2.size(), 2);
    EXPECT_NE(mgr2.TryGet<AutoSystem1>(), nullptr);
}

TEST(SystemsManagerMoveTest, Swap_ExchangesContents)
{
    auto mgr1 = SystemsManager::Create(RegistryRegistrator::Get("test1").systemRegIndexes);
    auto mgr2 = SystemsManager::Create(RegistryRegistrator::Get("test2").systemRegIndexes);

    const auto size1 = mgr1.size(); // 2
    const auto size2 = mgr2.size(); // 1

    mgr1.swap(mgr2);

    EXPECT_EQ(mgr1.size(), size2);
    EXPECT_EQ(mgr2.size(), size1);

    // After swap mgr1 has what mgr2 had (test2 = only AutoSystem2)
    EXPECT_NE(mgr1.TryGet<AutoSystem2>(), nullptr);
    EXPECT_EQ(mgr1.TryGet<AutoSystem1>(), nullptr);
}
