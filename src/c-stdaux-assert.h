/* There is no include guard. Just like <assert.h>, we can include this header
 * multiple times to update the macros for NDEBUG/C_MORE_ASSERT changes.
 *
 * The user can define NDEBUG to disable all asserts.
 *
 * The user can define C_MORE_ASSERT to a non-negative number to control
 * which assertions are enabled.
 */

#include <assert.h>
#include <c-stdaux-generic.h>

/**
 * C_MORE_ASSERT: user define to configure assertion levels (similar to NDEBUG).
 *
 * If NDEBUG is defined, then assert() is a nop. This also implies
 * C_MORE_ASSERT_LEVEL of zero, which means that c_more_assert() does not
 * evaluate the condition at runtime.
 *
 * Otherwise, if C_MORE_ASSERT is defined it determines the
 * C_MORE_ASSERT_LEVEL. If C_MORE_ASSERT, is undefined, C_MORE_ASSERT_LEVEL
 * defaults to 1.
 *
 * The effective C_MORE_ASSERT_LEVEL affects whether c_more_assert() and
 * c_more_assert_with() evaluates the condition at runtime. The purpose is that
 * more assertions are disabled by default (and in release builds). For
 * debugging and testing, define C_MORE_ASSERT to a number larger than 1.
 */
#undef C_MORE_ASSERT_LEVEL
#ifdef NDEBUG
#define C_MORE_ASSERT_LEVEL 0
#elif !defined(C_MORE_ASSERT)
#define C_MORE_ASSERT_LEVEL 1
#else
#define C_MORE_ASSERT_LEVEL (C_MORE_ASSERT)
#endif

#undef _c_assert_fail
#if C_MORE_ASSERT_LEVEL > 0 && defined(__GNU_LIBRARY__)
/* Depending on "_with_msg", we hide the "msg" string unless we build with
 * "C_MORE_ASSERT > 1". The point is to avoid embedding debugging strings in
 * the binary with release builds.
 *
 * The assertion failure messages are often not very useful for the end user
 * and for the developer __FILE__:__LINE__ is sufficient.
 *
 * __assert_fail() also exists on musl, but we don't have a separate detection
 * for musl.
 */
#define _c_assert_fail(_with_msg, msg)                                       \
    __assert_fail(                                                           \
        C_MORE_ASSERT_LEVEL > 1 || (_with_msg) ? "" msg "" : "<dropped>",    \
        __FILE__, __LINE__,                                                  \
        C_MORE_ASSERT_LEVEL > 1 || (_with_msg) ? "<unknown-fcn>" : __func__)
#else
#define _c_assert_fail(_with_msg, msg) \
    do {                               \
        assert(false && msg);          \
        _c_unreachable_code();         \
    } while (0)
#endif

/* There is an include guard. The remainder of this header is only evaluated
 * once upon multiple inclusions. */
#if !defined(C_HAS_STDAUX_ASSERT)
#define C_HAS_STDAUX_ASSERT

#if defined(C_COMPILER_GNUC)
#define _c_unreachable_code() __builtin_unreachable()
#else /* defined(C_COMPILER_GNUC) */
#define _c_unreachable_code()                                                 \
    do {                                                                      \
        /* Infinite loop without side effects is undefined behavior and marks \
         * unreachable code. */                                               \
    } while (1)
#endif /* defined(C_COMPILER_GNUC) */

#if defined(C_COMPILER_GNUC)
#define _c_assert_constant(_cond)                                            \
    do {                                                                     \
        if (__builtin_constant_p(_cond) && !(_cond)) {                       \
            /* With gcc, constant expressions are still evaluated and result \
             * in unreachable code too.                                      \
             *                                                               \
             * The point is to avoid compiler warnings with                  \
             * c_more_assert(false) and NDEBUG.                              \
             */                                                              \
            _c_unreachable_code();                                           \
        }                                                                    \
    } while (0)
#else /* defined(C_COMPILER_GNUC) */
#define _c_assert_constant(_cond) \
    do {                          \
        /* This does nothing. */  \
    } while (0)
#endif /* defined(C_COMPILER_GNUC) */

/**
 * c_more_assert_with() - Conditional runtime assertion.
 * @_level: Assertion level that determines whether the assertion is evaluated,
 *     based on comparison with C_MORE_ASSERT_LEVEL.
 * @_cond: Condition or expression to validate.
 *
 * This macro performs an assertion based on the specified _level in comparison
 * to the compile-time constant C_MORE_ASSERT_LEVEL. C_MORE_ASSERT_LEVEL
 * typically defaults to 1 but can be modified by defining NDEBUG or
 * C_MORE_ASSERT.
 *
 * - If _level is less than C_MORE_ASSERT_LEVEL, the condition is ignored and
 *   the assertion code is excluded from the final build, allowing for performance
 *   optimizations.
 *
 * - If _cond is a constant expression that fails, the compiler will mark the code
 *   path as unreachable, regardless of NDEBUG or the configured C_MORE_ASSERT_LEVEL.
 *
 * Unlike c_assert(), which always evaluates the condition,
 * `c_more_assert_with()` * only evaluates the condition if the specified _level *
 * meets the configured assertion threshold. This conditional behavior requires *
 * that _cond has no side effects, as it may not be evaluated in all cases.
 *
 * Note: This macro is usually excluded from regular builds unless explicitly
 * enabled by defining C_MORE_ASSERT, making it particularly useful for debugging
 * and testing without incurring runtime costs in production builds.
 *
 * The macro is async-signal-safe, if @_cond is and the assertion doesn't fail.
 */
#define c_more_assert_with(_level, _cond)                        \
    do {                                                         \
        /* c_more_assert_with() must do *nothing* of effect,     \
         * except evaluating @_cond (0 or 1 times).              \
         *                                                       \
         * As such, it is async-signal-safe (provided @_cond and \
         * @_level is, and the assertion does not fail). */      \
        if ((_level) < C_MORE_ASSERT_LEVEL) {                    \
            _c_assert_constant(_cond);                           \
        } else if (_c_likely_(_cond)) {                          \
            /* pass */                                           \
        } else {                                                 \
            _c_assert_fail(false, #_cond);                       \
        }                                                        \
    } while (0)

/**
 * c_more_assert() - Conditional runtime assertion.
 * @_cond: Condition or expression to validate.
 *
 * This is the same as c_more_assert_with(2, _cond). This means that
 * the assertion is usually disabled in regular builds unless the user
 * opts in by setting C_MORE_ASSERT to 2 or larger.
 *
 * The macro is async-signal-safe, if @_cond is and the assertion doesn't fail.
 */
#define c_more_assert(_cond) c_more_assert_with(2, _cond)

/**
 * c_assert() - Runtime assertions
 * @_cond:                 Result of an expression
 *
 * This function behaves like the standard ``assert(3)`` macro. That is, if
 * ``NDEBUG`` is defined, it is a no-op. In all other cases it will assert that
 * the result of the passed expression is true.
 *
 * Unlike the standard ``assert(3)`` macro, this function always evaluates its
 * argument. This means side-effects will always be evaluated! However, if the
 * macro is used with constant expressions, the compiler will be able to
 * optimize it away.
 *
 * The macro is async-signal-safe, if @_cond is and the assertion doesn't fail.
 */
#define c_assert(_cond)                    \
    do {                                   \
        if (!_c_likely_(_cond)) {          \
            _c_assert_fail(false, #_cond); \
        }                                  \
    } while (0)

/**
 * c_assert_not_reached() - Fail assertion when called.
 *
 * With C_COMPILER_GNUC, the macro calls assert(false) and marks the code
 * path as __builtin_unreachable(). The benefit is that also with NDEBUG the
 * compiler considers the path unreachable.
 *
 * Otherwise, just calls assert(false).
 *
 * The macro is async-signal-safe.
 */
#define c_assert_not_reached() _c_assert_fail(true, "unreachable")

#endif /* !defined(C_HAS_STDAUX_ASSERT) */

#ifdef __cplusplus
}
#endif
