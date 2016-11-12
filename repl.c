#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <editline/readline.h>
#include <editline/history.h>

#include "mpc.h"

int RIGHT = 0;
int LEFT = 1;

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

double eval_op(char* op, double x, double y) {
    if (strcmp(op, "+") == 0) return x + y;
    else if (strcmp(op, "-") == 0) return x - y;
    else if (strcmp(op, "*") == 0) return x * y;
    else if (strcmp(op, "/") == 0) return x / y;
    else if (strcmp(op, "^") == 0) return pow(x, y);
    else if (strcmp(op, "min") == 0) {
        if (x > y) return y;
        else return x;
    }
    else if (strcmp(op, "max") == 0) {
        if (x > y) return x;
        else return y;
    }
    else return NAN;
}

double eval_ast(mpc_ast_t* ast) {

    if(strstr(ast->tag, "number"))
        return atof(ast->contents);
    
    if (strstr(ast->tag, "expr")) {
        double x;
        int assoc;
        char* op = ast->children[1]->contents;

        if (strcmp(op, "^") == 0) assoc = RIGHT;
        else assoc = LEFT;

        if (assoc == LEFT) {
            x = eval_ast(ast->children[2]);
            for (int i=3; i < (ast->children_num - 1); i++)
                x = eval_op(op, x, eval_ast(ast->children[i]));
        } else {
            x = eval_ast(ast->children[(ast->children_num - 2)]);
            for (int i=(ast->children_num - 3); i > 1; i--)
                x = eval_op(op, eval_ast(ast->children[i]), x);
        }
        if (strcmp(op, "-") == 0 && ast->children_num == 4)
            x = 0 - x;
        return x;
    }
    else {
        // Must be root node, evaluate expr
        return eval_ast(ast->children[1]);
    }
}

int main(int argc, char** argv) {
    mpc_parser_t* Number    = mpc_new("number");
    mpc_parser_t* Operator  = mpc_new("operator");
    mpc_parser_t* Expr      = mpc_new("expr");
    mpc_parser_t* Statement = mpc_new("statement");

    mpca_lang(MPCA_LANG_DEFAULT,
        "                                                                     \
            number      : /-?[0-9]+([.][0-9]+)?/ ;                            \
            operator    : '+' | '-' | '/' | '*' | '^' | \"min\" | \"max\" ;   \
            expr        : <number> | '(' <operator> <expr>+ ')' ;             \
            statement   : /^/ <expr> /$/ ;                                    \
        ",
        Number, Operator, Expr, Statement
    );

    puts("jblisp version 0.1.0");
    puts("Press ^C to exit\n");

    mpc_result_t res;
    while (1) {
        char* input = readline("jblisp> ");
        // Parse input
        add_history(input);
        if (mpc_parse("<stdin>", input, Statement, &res)) {
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
