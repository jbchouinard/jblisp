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

    int run_repl=1;
    int argp;
    // Process CLI switches
    for (argp=1; argp < argc; argp++) {
        if (argv[argp][0] == '-' && argv[argp][1] == '-') {
            if (strcmp(argv[argp], "--stop") == 0) {
                run_repl=0;
            }
        }
        else { break; }
    }

    // Load CLI-specified files
    for (int i=argp; i < argc; i++) {
        exec_file(env, argv[i]);
    }

    while (run_repl) {
        char *input = readline("jblisp> ");
        if (strcmp(input, "(exit)")==0) { free(input); break; }
        add_history(input);
        exec_line(env, input);
        free(input);
    }

    mpc_cleanup(8, Boolean, Number, Symbol, String, Sexpr, Qexpr, Expr, JBLisp);
    return 0;
}
