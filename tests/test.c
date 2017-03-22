#include <stdio.h>
#include "../jblisp.h"
#include "../minunit.h"

int tests_run = 0;

static char * test_lenv_put() {
    lenv *e = lenv_new();
    lval *v = lval_lng(5);
    // lenv_put(e, "foo", lval_lng(5));
    return 0;
}

static char * all_tests() {
    mu_run_test(test_lenv_put);
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

