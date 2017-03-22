#include <editline/readline.h>
#include <editline/history.h>

#include "mpc.h"
#include "jblisp.h"

int main(int argc, char **argv) {
    build_parser();
    puts("jblisp version 0.2.3");
    puts("Press ^C to exit\n");

    for (int i=1; i < argc; i++) {
        exec_file(argv[i]);
    }

    while (1) {
        char *input = readline("jblisp> ");
        add_history(input);
        exec_line(input);
        free(input);
    }

    mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, JBLisp);
    return 0;
}
