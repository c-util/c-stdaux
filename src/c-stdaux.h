#pragma once

/*
 * Auxiliary macros and functions for the C standard library
 *
 * The `c-stdaux.h` header contains a collection of auxiliary macros and helper
 * functions around the functionality provided by the different C standard
 * library implementations, as well as other specifications implemented by
 * them.
 *
 * Most of the helpers provided here provide aliases for common library and
 * compiler features. Furthermore, several helpers simply provide other calling
 * conventions than their standard counterparts (e.g., they allow for NULL to
 * be passed with an object length of 0 where it makes sense to accept empty
 * input).
 *
 * The namespace used by this project is:
 *
 *  * `c_*` for all common C symbols or definitions that behave like proper C
 *    entities (e.g., macros that protect against double-evaluation would use
 *    lower-case names)
 *
 *  * `C_*` for all constants, as well as macros that may not be safe against
 *    double evaluation.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <stdalign.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/*
 * Shortcuts for gcc attributes. See GCC manual for details. They're 1-to-1
 * mappings to the GCC equivalents. No additional magic here. They are
 * supported by other compilers as well.
 */
#define _c_cleanup_(_x) __attribute__((__cleanup__(_x)))
#define _c_const_ __attribute__((__const__))
#define _c_deprecated_ __attribute__((__deprecated__))
#define _c_hidden_ __attribute__((__visibility__("hidden")))
#define _c_likely_(_x) (__builtin_expect(!!(_x), 1))
#define _c_packed_ __attribute__((__packed__))
#define _c_printf_(_a, _b) __attribute__((__format__(printf, _a, _b)))
#define _c_public_ __attribute__((__visibility__("default")))
#define _c_pure_ __attribute__((__pure__))
#define _c_sentinel_ __attribute__((__sentinel__))
#define _c_unlikely_(_x) (__builtin_expect(!!(_x), 0))
#define _c_unused_ __attribute__((__unused__))

/**
 * c_assert() - runtime assertions
 * @expr_result:                result of an expression
 *
 * This function behaves like the standard `assert(3)` macro. That is, if
 * `NDEBUG` is defined, it is a no-op. In all other cases it will assert that
 * the result of the passed expression is true.
 *
 * Unlike the standard `assert(3)` macro, this function always evaluates its
 * argument. This means side-effects will always be evaluated! However, if the
 * macro is used with constant expressions, the compiler will be able to
 * optimize it away.
 */
static inline void c_assert(bool expr_result) {
        assert(expr_result);
}

/**
 * c_errno() - return valid errno
 *
 * This helper should be used to shut up gcc if you know 'errno' is valid (ie.,
 * errno is > 0). Instead of "return -errno;", use
 * "return -c_errno();" It will suppress bogus gcc warnings in case it assumes
 * 'errno' might be 0 (or <0) and thus the caller's error-handling might not be
 * triggered.
 *
 * This helper should be avoided whenever possible. However, occasionally we
 * really want to shut up gcc (especially with static/inline functions). In
 * those cases, gcc usually cannot deduce that some error paths are guaranteed
 * to be taken. Hence, making the return value explicit allows gcc to better
 * optimize the code.
 *
 * Note that you really should never use this helper to work around broken libc
 * calls or syscalls, not setting 'errno' correctly.
 *
 * Return: Positive error code is returned.
 */
static inline int c_errno(void) {
        return _c_likely_(errno > 0) ? errno : ENOTRECOVERABLE;
}

/*
 * Common Destructors
 *
 * Followingly, there're a bunch of common 'static inline' destructors, which
 * simply call the function that they're named after, but return "INVALID"
 * instead of "void". This allows direct assignment to any member-field and/or
 * variable they're defined in, like:
 *
 *   foo = c_free(foo);
 *
 * or
 *
 *   foo->bar = c_close(foo->bar);
 *
 * Furthermore, all those destructors can be safely called with the "INVALID"
 * value as argument, and they will be a no-op.
 */

static inline void *c_free(void *p) {
        free(p);
        return NULL;
}

static inline int c_close(int fd) {
        if (fd >= 0)
                close(fd);
        return -1;
}

static inline FILE *c_fclose(FILE *f) {
        if (f)
                fclose(f);
        return NULL;
}

static inline DIR *c_closedir(DIR *d) {
        if (d)
                closedir(d);
        return NULL;
}

/*
 * Common Cleanup Helpers
 *
 * A bunch of _c_cleanup_(foobarp) helpers that are used all over the place.
 * Note that all of those have the "if (IS_INVALID(foobar))" check inline, so
 * compilers can optimize most of the cleanup-paths in a function. However, if
 * the function they call already does this _inline_, then it might be skipped.
 */

#define C_DEFINE_CLEANUP(_type, _func)                                          \
        static inline void _func ## p(_type *p) {                               \
                if (*p)                                                         \
                        _func(*p);                                              \
        } struct c_internal_trailing_semicolon

#define C_DEFINE_DIRECT_CLEANUP(_type, _func)                                   \
        static inline void _func ## p(_type *p) {                               \
                _func(*p);                                                      \
        } struct c_internal_trailing_semicolon

static inline void c_freep(void *p) {
        /*
         * `foobar **` does not coerce to `void **`, so we need `void *` as
         * argument type, and then we dereference manually.
         */
        c_free(*(void **)p);
}

C_DEFINE_DIRECT_CLEANUP(int, c_close);
C_DEFINE_CLEANUP(FILE *, c_fclose);
C_DEFINE_CLEANUP(DIR *, c_closedir);

#ifdef __cplusplus
}
#endif
