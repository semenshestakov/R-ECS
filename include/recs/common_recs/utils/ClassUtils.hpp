#pragma once


/**
 * @def DEEP_TEST_PRIVATE_ACCESS
 * @brief Controls access to private members during testing
 *
 * This macro changes class member access control based on test mode.
 * When DEEP_TEST_ENABLE is defined, private members become public for testing.
 * Otherwise, they remain private.
 *
 * Usage example:
 * @code
 * class MyClass
 * {
 * DEEP_TEST_PRIVATE_ACCESS:
 *     void internalFunction(); // private in release, public in tests
 * };
 * @endcode
 *
 * @note Define DEEP_TEST_ENABLE before including this header to enable test access
 * @see DEEP_TEST_PROTECTED_ACCESS
 */
#ifdef DEEP_TEST_ENABLE
#define DEEP_TEST_PRIVATE_ACCESS public
#else
#define DEEP_TEST_PRIVATE_ACCESS private
#endif


/**
 * @def DEEP_TEST_PROTECTED_ACCESS
 * @brief Controls access to protected members during testing
 *
 * This macro changes class member access control based on test mode.
 * When DEEP_TEST_ENABLE is defined, protected members become public for testing.
 * Otherwise, they remain protected.
 *
 * Usage example:
 * @code
 * class BaseClass {
 * DEEP_TEST_PROTECTED_ACCESS:
 *     void helperFunction(); // protected in release, public in tests
 * };
 *
 * class DerivedClass : public BaseClass
 * {
 *     void useHelper() { helperFunction(); } // always accessible
 * };
 * @endcode
 *
 * @note Define DEEP_TEST_ENABLE before including this header to enable test access
 * @see DEEP_TEST_PRIVATE_ACCESS
 */
#ifdef DEEP_TEST_ENABLE
#define DEEP_TEST_PROTECTED_ACCESS public
#else
#define DEEP_TEST_PROTECTED_ACCESS protected
#endif

