#include <stdio.h>
#include "../jblisp.h"
#include "../minunit.h"

int tests_run = 0;

static char *test_lval() {
    lval *v = lval_lng(10);
    lval *w = lval_lng(10);
    mu_assert(lval_is(v, v), "An LVAL_LNG is not itself?");
    mu_assert(lval_equal(v, v), "An LVAL_LNG is not equal to itself?");
    mu_assert(lval_equal(v, w), "LVAL_LNGs of same value should be equal.");
    lval_del(v);
    lval_del(w);
    v = lval_dbl(10.0);
    w = lval_dbl(10.0);
    mu_assert(lval_is(v, v), "An LVAL_DBL is not itself?");
    mu_assert(lval_equal(v, v), "An LVAL_DBL is not equal to itself?");
    mu_assert(lval_equal(v, w), "LVAL_DBL of same value should be equal.");
    lval_del(v);
    lval_del(w);
    v = lval_sym("foo");
    w = lval_sym("foo");
    mu_assert(lval_is(v, v), "An LVAL_SYM is not itself?");
    mu_assert(lval_equal(v, v), "An LVAL_SYM is not equal to itself?");
    mu_assert(lval_equal(v, w), "LVAL_SYM of same value should be equal.");
    lval_del(v);
    lval_del(w);
    return 0;
}

static char *test_lenv() {
    lenv *e = lenv_new();
    lval *v = lval_lng(5);
    lenv_put(e, "foo", v);
    lval *w = lenv_get(e, "foo");
    mu_assert(lval_equal(v, w), "GET: Did not get back what we put in the env.");
    lval_del(v);
    lval_del(w);
    lenv_del(e);
    return 0;
}

static char *all_tests() {
    mu_run_test(test_lval);
    mu_run_test(test_lenv);
    return 0;
}

int main(int argc, char **argv) {
    char *result = all_tests();
    if (result != 0) {
        printf("%s\n", result);
    }
    else {
        printf("ALL TESTS PASSED\n");
    }
    printf("TESTS RUN: %d\n", tests_run);

    return result != 0;
}

