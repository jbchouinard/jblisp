#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <editline/readline.h>
#include <editline/history.h>

#include "mpc.h"


int count_nodes(mpc_ast_t* ast) {
	if (ast->children_num == 0)
        return 1;
    else {
        int total = 1;
        for (int i = 0; i < ast->children_num; i++) {
            total += count_nodes(ast->children[i]);
        }
        return total;
    }
    return 0;
}

double eval_ast(mpc_ast_t* ast) {
    if (ast->children_num) {
        if (strstr(ast->tag, "expr")) {
            char* op = ast->children[1]->contents;
            if (strcmp(op, "+") == 0) {
                double total = 0;
                for (int i=2; i < (ast->children_num - 1); i++) {
                    total += eval_ast(ast->children[i]);
                }
                return total;
            } else if (strcmp(op, "*") == 0) {
                double total = 1;
                for (int i=2; i < (ast->children_num - 1); i++) {
                    total *= eval_ast(ast->children[i]);
                }
                return total;
            } else if (strcmp(op, "-") == 0) {
                double total = eval_ast(ast->children[2]);
                for (int i=3; i < (ast->children_num - 1); i++) {
                    total -= eval_ast(ast->children[i]);
                }
                return total;
            } else if (strcmp(op, "/") == 0) {
                double total = eval_ast(ast->children[2]);
                for (int i=3; i < (ast->children_num - 1); i++) {
                    total /= eval_ast(ast->children[i]);
                }
                return total;
            } else {
                return NAN;
            }
        } else {
            // Must be root node, evaluate expr
            return eval_ast(ast->children[1]);
        }
    }
    else {
        if (strstr(ast->tag, "number")) {
            return atof(ast->contents);
        } else {
            return NAN;
        }
    }
}

int main(int argc, char** argv) {
    mpc_parser_t* Number    = mpc_new("number");
    mpc_parser_t* Operator  = mpc_new("operator");
    mpc_parser_t* Expr      = mpc_new("expr");
    mpc_parser_t* JBLisp    = mpc_new("jblisp");

    mpca_lang(MPCA_LANG_DEFAULT,
        "                                                                     \
            number      : /-?[0-9]+([.][0-9]+)?/ ;                            \
            operator    : '+' | '-' | '/' | '*' ;                             \
            expr        : <number> | '(' <operator> <expr> <expr>+ ')' ;      \
            jblisp      : /^/ <expr> /$/ ;                                    \
        ",
        Number, Operator, Expr, JBLisp
    );

    puts("jblisp version 0.1.0");
    puts("Press ^C to exit\n");

    mpc_result_t res;
    while (1) {
        char* input = readline("jblisp> ");
        // Parse input
        add_history(input);
        if (mpc_parse("<stdin>", input, JBLisp, &res)) {
            mpc_ast_print(res.output);
            mpc_ast_t* a = res.output;
            printf("%f\n", eval_ast(a));
            mpc_ast_delete(res.output);
        } else {
            mpc_err_print(res.error);
            mpc_err_delete(res.error);
        }
        free(input);
    }
    return 0;
}
