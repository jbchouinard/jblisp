#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>
#include <editline/history.h>

#include "mpc.h"

int main(int argc, char** argv) {
    mpc_parser_t* Integer   = mpc_new("integer");
    mpc_parser_t* Operator  = mpc_new("operator");
    mpc_parser_t* Expr      = mpc_new("expr");
    mpc_parser_t* JBLisp    = mpc_new("jblisp");

    mpca_lang(MPCA_LANG_DEFAULT,
        "                                                               \
            integer     : /-?[0-9]+/ ;                                  \
            operator    : '+' | '-' | '/' | '*' ;                       \
            expr        : <integer> | '(' <operator> <expr>+ ')' ;      \
            jblisp      : /^(/<expr>/\n|;)+$/;                          \
        ",
        Integer, Operator, Expr, JBLisp
    );

    puts("jblisp version 0.0.1");
    puts("Press ^C to exit\n");

    mpc_result_t res;
    while (1) {
        char* input = readline("jblisp> ");
        // Parse input
        add_history(input);
        if (mpc_parse("<stdin>", input, JBLisp, &res)) {
            mpc_ast_t* a = res.output;
            printf("Tag: %s\n", a->tag);
            printf("Contents: %s\n", a->contents);
            printf("Number of children: %i\n", a->children_num);
            mpc_ast_print(res.output);
            mpc_ast_delete(res.output);
        } else {
            mpc_err_print(res.error);
            mpc_err_delete(res.error);
        }
        free(input);
    }
    return 0;
}
