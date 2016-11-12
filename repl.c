#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>
#include <editline/history.h>

#include "mpc.h"


int number_of_nodes(mpc_ast_t* ast) {
	if (ast->children_num == 0)
        return 1;
    else {
        int total = 1;
        for (int i = 0; i < ast->children_num; i++) {
            total += number_of_nodes(ast->children[i]);
        }
        return total;
    }
    return 0;
}

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
            jblisp      : /^/ <expr>+ /$/ ;                             \
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
            mpc_ast_print(res.output);
            mpc_ast_t* a = res.output;
            printf("Number of nodes: %i\n", number_of_nodes(a));
            mpc_ast_delete(res.output);
        } else {
            mpc_err_print(res.error);
            mpc_err_delete(res.error);
        }
        free(input);
    }
    return 0;
}
