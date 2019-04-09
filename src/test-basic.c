/*
 * Tests for Basic Functionality
 *
 * This runs same basic verification that each feature does what we expect it
 * to do. More elaborate tests and/or stress-tests are not included here.
 */

#include <stdlib.h>
#include <sys/eventfd.h>
#include "c-stdaux.h"

/*
 * Tests for all remaining helpers
 */
static void test_misc(int non_constant_expr) {
        int foo;

        /*
         * Test the C_EXPR_ASSERT() macro to work in static and non-static
         * environments, and evaluate exactly to its passed expression.
         */
        {
                static int v = C_EXPR_ASSERT(1, true, "");

                assert(v == 1);
        }

        /*
         * Test stringify/concatenation helpers. Also make sure to test that
         * the passed arguments are evaluated first, before they're stringified
         * and/or concatenated.
         */
        {
#define TEST_TOKEN foobar
                assert(!strcmp("foobar", C_STRINGIFY(foobar)));
                assert(!strcmp("foobar", C_STRINGIFY(TEST_TOKEN)));
                assert(!strcmp("foobar", C_STRINGIFY(C_CONCATENATE(foo, bar))));
                assert(!strcmp("foobarfoobar", C_STRINGIFY(C_CONCATENATE(TEST_TOKEN, foobar))));
                assert(!strcmp("foobarfoobar", C_STRINGIFY(C_CONCATENATE(foobar, TEST_TOKEN))));
#undef TEST_TOKEN
        }

        /*
         * Test tuple expansion. This is used to strip tuple-wrappers in the
         * pre-processor.
         * We make sure that it works with {0,1,2}-tuples, as well as only
         * strips a single layer.
         */
        {
                /*
                 * strcmp() might be a macro, so make sure we get a proper C
                 * expression below. Otherwise, C_EXPAND() cannot be used that
                 * way (since it would evaluate to a single macro argument).
                 */
                int (*f) (const char *, const char *) = strcmp;

                assert(!f(C_EXPAND(()) "foobar", "foo" "bar"));
                assert(!f(C_EXPAND(("foobar")), "foo" "bar"));
                assert(!f(C_EXPAND(("foobar", "foo" "bar"))));
                assert(!f C_EXPAND((("foobar", "foo" "bar"))));
        }

        /*
         * Test C_VAR() macro. It's sole purpose is to create a valid C
         * identifier given a single argument (which itself must be a valid
         * identifier).
         * Just test that we can declare variables with it and use it in
         * expressions.
         */
        {
                {
                        int C_VAR(sub, UNIQUE) = 5;
                        /* make sure the variable name does not clash */
                        int sub = 12, subUNIQUE = 12, UNIQUEsub = 12;

                        assert(7 + C_VAR(sub, UNIQUE) == sub);
                        assert(sub == subUNIQUE);
                        assert(sub == UNIQUEsub);
                }
                {
                        /*
                         * Make sure both produce different names, even though they're
                         * exactly the same expression.
                         */
                        _c_unused_ int C_VAR(sub, __COUNTER__), C_VAR(sub, __COUNTER__);
                }
                {
                        /* verify C_VAR() with single argument works line-based */
                        int C_VAR(sub); C_VAR(sub) = 5; assert(C_VAR(sub) == 5);
                }
                {
                        /* verify C_VAR() with no argument works line-based */
                        int C_VAR(); C_VAR() = 5; assert(C_VAR() == 5);
                }
        }

        /*
         * Test array-size helper. This simply computes the number of elements
         * of an array, instead of the binary size.
         */
        {
                int bar[8];

                static_assert(C_ARRAY_SIZE(bar) == 8, "");
                assert(__builtin_constant_p(C_ARRAY_SIZE(bar)));
        }

        /*
         * Test decimal-representation calculator. Make sure it is
         * type-independent and just uses the size of the type to calculate how
         * many bytes are needed to print that integer in decimal form. Also
         * verify that it is a constant expression.
         */
        {
                static_assert(C_DECIMAL_MAX(char) == 4, "");
                static_assert(C_DECIMAL_MAX(signed char) == 4, "");
                static_assert(C_DECIMAL_MAX(unsigned char) == 4, "");
                static_assert(C_DECIMAL_MAX(unsigned long) == (sizeof(long) == 8 ? 21 : 11), "");
                static_assert(C_DECIMAL_MAX(unsigned long long) == 21, "");
                static_assert(C_DECIMAL_MAX(int32_t) == 11, "");
                static_assert(C_DECIMAL_MAX(uint32_t) == 11, "");
                static_assert(C_DECIMAL_MAX(uint64_t) == 21, "");
        }

        /*
         * Test c_container_of(). We cannot test for type-safety, nor for
         * other invalid uses, as they'd require negative compile-testing.
         * However, we can test that the macro yields the correct values under
         * normal use.
         */
        {
                struct foobar {
                        int a;
                        char b;
                } sub = {};

                assert(&sub == c_container_of(&sub.a, struct foobar, a));
                assert(&sub == c_container_of(&sub.b, struct foobar, b));
                assert(&sub == c_container_of((const char *)&sub.b, struct foobar, b));
        }

        /*
         * Test min/max macros. Especially check that macro arguments are never
         * evaluated multiple times, and if both arguments are constant, the
         * return value is constant as well.
         */
        {
                foo = 0;
                assert(c_max(1, 5) == 5);
                assert(c_max(-1, 5) == 5);
                assert(c_max(-1, -5) == -1);
                assert(c_max(foo++, -1) == 0);
                assert(foo == 1);
                assert(c_max(foo++, foo++) > 0);
                assert(foo == 3);

                assert(__builtin_constant_p(c_max(1, 5)));
                assert(!__builtin_constant_p(c_max(1, non_constant_expr)));

                foo = 0;
                assert(c_min(1, 5) == 1);
                assert(c_min(-1, 5) == -1);
                assert(c_min(-1, -5) == -5);
                assert(c_min(foo++, 1) == 0);
                assert(foo == 1);
                assert(c_min(foo++, foo++) > 0);
                assert(foo == 3);

                assert(__builtin_constant_p(c_min(1, 5)));
                assert(!__builtin_constant_p(c_min(1, non_constant_expr)));
        }

        /*
         * Test c_less_by(), c_clamp(). Make sure they
         * evaluate arguments exactly once, and yield a constant expression,
         * if all arguments are constant.
         */
        {
                foo = 8;
                assert(c_less_by(1, 5) == 0);
                assert(c_less_by(5, 1) == 4);
                assert(c_less_by(foo++, 1) == 7);
                assert(foo == 9);
                assert(c_less_by(foo++, foo++) >= 0);
                assert(foo == 11);

                assert(__builtin_constant_p(c_less_by(1, 5)));
                assert(!__builtin_constant_p(c_less_by(1, non_constant_expr)));

                foo = 8;
                assert(c_clamp(foo, 1, 5) == 5);
                assert(c_clamp(foo, 9, 20) == 9);
                assert(c_clamp(foo++, 1, 5) == 5);
                assert(foo == 9);
                assert(c_clamp(foo++, foo++, foo++) >= 0);
                assert(foo == 12);

                assert(__builtin_constant_p(c_clamp(0, 1, 5)));
                assert(!__builtin_constant_p(c_clamp(1, 0, non_constant_expr)));
        }

        /*
         * Div Round Up: Normal division, but round up to next integer, instead
         * of clipping. Also verify that it does not suffer from the integer
         * overflow in the prevalant, alternative implementation:
         *      [(x + y - 1) / y].
         */
        {
                int i, j;

#define TEST_ALT_DIV(_x, _y) (((_x) + (_y) - 1) / (_y))
                foo = 8;
                assert(c_div_round_up(0, 5) == 0);
                assert(c_div_round_up(1, 5) == 1);
                assert(c_div_round_up(5, 5) == 1);
                assert(c_div_round_up(6, 5) == 2);
                assert(c_div_round_up(foo++, 1) == 8);
                assert(foo == 9);
                assert(c_div_round_up(foo++, foo++) >= 0);
                assert(foo == 11);

                assert(__builtin_constant_p(c_div_round_up(1, 5)));
                assert(!__builtin_constant_p(c_div_round_up(1, non_constant_expr)));

                /* alternative calculation is [(x + y - 1) / y], but it may overflow */
                for (i = 0; i <= 0xffff; ++i) {
                        for (j = 1; j <= 0xff; ++j)
                                assert(c_div_round_up(i, j) == TEST_ALT_DIV(i, j));
                        for (j = 0xff00; j <= 0xffff; ++j)
                                assert(c_div_round_up(i, j) == TEST_ALT_DIV(i, j));
                }

                /* make sure it doesn't suffer from high overflow */
                assert(UINT32_C(0xfffffffa) % 10 == 0);
                assert(UINT32_C(0xfffffffa) / 10 == UINT32_C(429496729));
                assert(c_div_round_up(UINT32_C(0xfffffffa), 10) == UINT32_C(429496729));
                assert(TEST_ALT_DIV(UINT32_C(0xfffffffa), 10) == 0); /* overflow */

                assert(UINT32_C(0xfffffffd) % 10 == 3);
                assert(UINT32_C(0xfffffffd) / 10 == UINT32_C(429496729));
                assert(c_div_round_up(UINT32_C(0xfffffffd), 10) == UINT32_C(429496730));
                assert(TEST_ALT_DIV(UINT32_C(0xfffffffd), 10) == 0);
#undef TEST_ALT_DIV
        }

        /*
         * Test c_assert(). Make sure side-effects are always evaluated, and
         * variables are marked as used regardless of NDEBUG.
         */
        {
                int v1 = 0, v2 = 0;

#define NDEBUG 1
                c_assert(!v1);
#undef NDEBUG
                c_assert(!v2);
        }

        /*
         * Test c_errno(). Simply verify that the correct value is returned. It
         * must always be >0 and equivalent to `errno' if set.
         */
        {
                assert(c_errno() > 0);

                close(-1);
                assert(c_errno() == errno);

                errno = 0;
                assert(c_errno() != errno);
        }
}

/*
 * Tests for:
 *  - c_free*()
 *  - c_close*()
 *  - c_fclose*()
 *  - c_closedir*()
 */
static void test_destructors(void) {
        int i;

        /*
         * Verify that c_free*() works as expected. Since we want to support
         * running under valgrind, there is no easy way to verify the
         * correctness of free(). Hence, we simply rely on valgrind to catch
         * the leaks.
         */
        {
                for (i = 0; i < 16; ++i) {
                        _c_cleanup_(c_freep) void *foo;
                        _c_cleanup_(c_freep) int **bar; /* supports any type */
                        size_t sz = 128 * 1024;

                        foo = malloc(sz);
                        assert(foo);

                        bar = malloc(sz);
                        assert(bar);
                        bar = c_free(bar);
                        assert(!bar);
                }

                assert(c_free(NULL) == NULL);
        }

        /*
         * Test c_close*(), rely on sparse FD allocation. Make sure all the
         * helpers actually close the fd, and cope fine with negative numbers.
         */
        {
                int fd;

                fd = eventfd(0, EFD_CLOEXEC);
                assert(fd >= 0);

                /* verify c_close() returns -1 */
                assert(c_close(fd) == -1);

                /* verify c_close() deals fine with negative fds */
                assert(c_close(-1) == -1);
                assert(c_close(-16) == -1);

                /* make sure c_closep() deals fine with negative FDs */
                {
                        _c_cleanup_(c_closep) int t = 0;
                        t = -1;
                }

                /*
                 * Make sure the c_close() earlier worked, by allocating the
                 * FD again and relying on the same FD number to be reused. Do
                 * this twice, to verify that the c_closep() in the cleanup
                 * path works as well.
                 */
                for (i = 0; i < 2; ++i) {
                        _c_cleanup_(c_closep) int t = -1;

                        t = eventfd(0, EFD_CLOEXEC);
                        assert(t >= 0);
                        assert(t == fd);
                }
        }

        /*
         * Test c_fclose() and c_fclosep(). This uses the same logic as the
         * tests for c_close() (i.e., sparse FD allocation).
         */
        {
                FILE *f;
                int fd;

                fd = eventfd(0, EFD_CLOEXEC);
                assert(fd >= 0);

                f = fdopen(fd, "r");
                assert(f);

                /* verify c_fclose() returns NULL */
                f = c_fclose(f);
                assert(!f);

                /* verify c_fclose() deals fine with NULL */
                assert(!c_fclose(NULL));

                /* make sure c_flosep() deals fine with NULL */
                {
                        _c_cleanup_(c_fclosep) FILE *t = (void *)0xdeadbeef;
                        t = NULL;
                }

                /*
                 * Make sure the c_fclose() earlier worked, by allocating the
                 * FD again and relying on the same FD number to be reused. Do
                 * this twice, to verify that the c_fclosep() in the cleanup
                 * path works as well.
                 */
                for (i = 0; i < 2; ++i) {
                        _c_cleanup_(c_fclosep) FILE *t = NULL;
                        int tfd;

                        tfd = eventfd(0, EFD_CLOEXEC);
                        assert(tfd >= 0);
                        assert(tfd == fd); /* the same as before */

                        t = fdopen(tfd, "r");
                        assert(t);
                }
        }
}

int main(int argc, char **argv) {
        test_misc(argc);
        test_destructors();
        return 0;
}
