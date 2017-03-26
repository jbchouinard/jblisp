#include <editline/readline.h>
#include <editline/history.h>

#include "mpc.h"
#include "jblisp.h"

int main(int argc, char **argv) {
    lenv *env = lenv_new(NULL);
    add_builtins(env);
    build_parser();
    puts("jblisp version " VERSION);
    puts("Press ^C to exit\n");

    // Load language
    exec_file(env, "lang/base.jbl");

    // Load CLI-specified files
    for (int i=1; i < argc; i++) {
        exec_file(env, argv[i]);
    }

    while (1) {
        char *input = readline("jblisp> ");
        add_history(input);
        exec_line(env, input);
        free(input);
    }

    mpc_cleanup(8, Boolean, Number, Symbol, String, Sexpr, Qexpr, Expr, JBLisp);
    return 0;
}
