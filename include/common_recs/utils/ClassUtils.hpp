#pragma once


#ifdef DEEP_TEST_ENABLE
#define DEEP_TEST_PRIVATE_ACCESS public
#else
#define DEEP_TEST_PRIVATE_ACCESS private
#endif

#ifdef DEEP_TEST_ENABLE
#define DEEP_TEST_PROTECTED_ACCESS public
#else
#define DEEP_TEST_PROTECTED_ACCESS protected
#endif

