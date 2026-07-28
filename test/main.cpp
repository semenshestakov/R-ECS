#include <gmock/gmock.h>
#include "ecs/jobs/ThreadAffinity.hpp"


// Designate the thread that runs the tests as the ECS main thread before any
// test body executes, so the ECS_ASSERT_MAIN_THREAD affinity checks are active.
int main(int argc, char** argv)
{
    ecs::MarkMainThread();
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}
