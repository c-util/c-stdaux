/* No include guard. Just like <assert.h>, we can include the header multiple
 * times to update the macros for NDEBUG/C_MORE_ASSERTS changes.
 *
 * The user can define NDEBUG to disable all asserts.
 *
 * The user can define C_MORE_ASSERTS to a non-negative number.
 *   - defined(NDEBUG) implies C_MORE_ASSERTS 0
 *   - C_MORE_ASSERTS 0 means asserts are disabled (like NDEBUG)
 *   - C_MORE_ASSERTS 1 is the default, and assert() and c_assert() is enabled.
 *   - C_MORE_ASSERTS > 1 means that c_more_assert() is enabled, based on the level. */

#include <c-stdaux-generic.h>
#include <assert.h>

/**
 * C_MORE_ASSERTS_LEVEL: the detected assertion level. Depends on NDEBUG and
 * C_MORE_ASSERTS define. */
#undef C_MORE_ASSERTS_LEVEL
#ifdef NDEBUG
#  define C_MORE_ASSERTS_LEVEL 0
#elif !defined(C_MORE_ASSERTS)
#  define C_MORE_ASSERTS_LEVEL 1
#else
#  define C_MORE_ASSERTS_LEVEL (C_MORE_ASSERTS)
#endif

/*****************************************************************************/

#undef _c_assert_fail
#if !defined(C_COMPILER_GNUC)
#define _c_assert_fail(drop_msg, msg) assert(false && msg)
#elif C_MORE_ASSERTS_LEVEL <= 0
#define _c_assert_fail(drop_msg, msg) _c_unreachable_code()
#elif defined(__GNU_LIBRARY__)
 /* __assert_fail() also exists on musl, but we don't detect that.
  *
  * Depending on "drop_msg", we hide the "msg" unless we build with
  * "C_MORE_ASSERTS > 1". The reason is that an assertion failure is not useful
  * for the end user, and for the developer the __FILE__:__LINE__ is
  * sufficient. The __func__ is dropped unless "C_MORE_ASSERTS > 1".
  * The point is to not embed many debugging strings in the binary. */
#define _c_assert_fail(drop_msg, msg)                                           \
        __assert_fail((drop_msg) && C_MORE_ASSERTS_LEVEL < 1                    \
                            ? "<dropped>"                                       \
                            : ""msg"",                                          \
                      __FILE__,                                                 \
                      __LINE__,                                                 \
                      C_MORE_ASSERTS_LEVEL < 1                                  \
                            ?  "<unknown-fcn>"                                  \
                            : __func__)
#else
#define _c_assert_fail(drop_msg, msg) ((void) assert(false && msg), _c_unreachable_code())
#endif

/*****************************************************************************/

/* The remainder we only define once (upon multiple inclusions) */
#if !defined(C_HAS_STDAUX_ASSERT)
#define C_HAS_STDAUX_ASSERT

#if defined(C_COMPILER_GNUC)

#define _c_unreachable_code() __builtin_unreachable()

#define c_assert_nse_on(_level, _cond)                                          \
        do {                                                                    \
                /* c_assert_nse_on() must do *nothing* of effect,               \
                 * except evaluating @_cond (0 or 1 times).                     \
                 *                                                              \
                 * As such, it is async-signal-safe (provided @_cond and        \
                 * @_level is, and the assertion does not fail). */             \
                if ((_level) < C_MORE_ASSERTS_LEVEL) {                          \
                    if (__builtin_constant_p(_cond) && !(_cond)) {              \
                        /* Constant expressions are still evaluated and result  \
                         * in unreachable code too.                             \
                         *                                                      \
                         * This can avoid compiler warnings about unreachable   \
                         * code with c_assert_nse(false).                       \
                         */                                                     \
                        _c_unreachable_code();                                  \
                    }                                                           \
                    /* pass */                                                  \
                } else if (_c_likely_(_cond)) {                                 \
                    /* pass */                                                  \
                } else {                                                        \
                    _c_assert_fail(true, #_cond);                               \
                }                                                               \
        } while (0)

#define c_assert_nse(_cond)  c_assert_nse_on(1, _cond)
#define c_assert_nse2(_cond) c_assert_nse_on(2, _cond)

#endif /* defined(C_COMPILER_GNUC) */

/**
 * c_assert() - Runtime assertions
 * @_x:                 Result of an expression
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
 * The macro is async-signal-safe, if @_x is and the assertion doesn't fail.
 */
#define c_assert(_cond)                                                         \
        do {                                                                    \
                if (!_c_likely_(_cond)) {                                       \
                        _c_assert_fail(true, #_cond);                           \
                }                                                               \
        } while (0)

/**
 * c_assert_not_reached() - Fail assertion when called.
 *
 * With C_COMPILER_GNUC, the macro calls assert(false) and marks the code
 * path as __builtin_unreachable(). The benefit is that also with NDEBUG the
 * compiler considers the path unreachable.
 *
 * Otherwise, just calls assert(false).
 */
#define c_assert_not_reached() _c_assert_fail(false, "unreachable")

#endif /* !defined(C_HAS_STDAUX_ASSERT) */

#ifdef __cplusplus
}
#endif
