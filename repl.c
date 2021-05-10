#include <editline/readline.h>

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

#ifdef JBLISPC_DEBUG_MEM
    for (int i=0; i < (COUNT_LENVNEW+COUNT_LENVCPY); i++) {
        lenv_del(LENVS[i]);
    }

    printf("LVALs created: %li\n", COUNT_LVALNEW);
    printf("LVALs copied: %li\n", COUNT_LVALCPY);
    printf("LVALs deleted: %li\n", COUNT_LVALDEL);
    printf("LVALs left: %li\n\n", COUNT_LVALNEW+COUNT_LVALCPY-COUNT_LVALDEL);
    printf("LPROCs created: %li\n", COUNT_LPROCNEW);
    printf("LPROCs copied: %li\n", COUNT_LPROCCPY);
    printf("LPROCs deleted: %li\n", COUNT_LPROCDEL);
    printf("LPROCs left: %li\n\n", COUNT_LPROCNEW+COUNT_LPROCCPY-COUNT_LPROCDEL);
    printf("LENVs created: %li\n", COUNT_LENVNEW);
    printf("LENVs copied: %li\n", COUNT_LENVCPY);
    printf("LENVs deleted: %li\n", COUNT_LENVDEL);
    printf("LENVs left: %li\n\n", COUNT_LENVNEW+COUNT_LENVCPY-COUNT_LENVDEL);
#endif

    cleanup_parser();
    return 0;
}
