#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <editline/readline.h>
#include <editline/history.h>

#include "mpc.h"

// Lisp Value (lval) type
typedef struct {
    enum { LVAL_LNG, LVAL_DBL, LVAL_ERR } type;
    union {
        double dbl;
        long lng;
        enum { LERR_BAD_OP, LERR_BAD_NUM, LERR_BAD_TYPE } err;
    } val;
} lval;

// Operator association types
enum { ASSOC_RIGHT, ASSOC_LEFT };

lval lval_dbl(double x) {
    lval v;
    v.type = LVAL_DBL;
    v.val.dbl = x;
    return v;
}

lval lval_lng(long x) {
    lval v;
    v.type = LVAL_LNG;
    v.val.lng = x;
    return v;
}

lval lval_err(int x) {
    lval v;
    v.type = LVAL_ERR;
    v.val.err = x;
    return v;
}

void print_lval(lval v) {
    switch (v.type) {
        case LVAL_LNG:
            printf("%li", v.val.lng);
            break;
        case LVAL_DBL:
            printf("%lf", v.val.dbl);
            break;
        case LVAL_ERR:
            switch(v.val.err) {
                case LERR_BAD_OP:
                    printf("Error: Invalid operator.");
                    break;
                case LERR_BAD_NUM:
                    printf("Error: Invalid number.");
                    break;
                case LERR_BAD_TYPE:
                    printf("Error: A type error occured.");
                    break;
                default:
                    printf("Error: A strange error occured.");
                    break;
            }
            break;
        default:
            printf("Error: Lval has invalid type.");
            break;
    }
}

void println_lval(lval v) {
    print_lval(v);
    putchar('\n');
}

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

lval eval_op(char* op, lval x, lval y) {
    if (x.type == LVAL_ERR) return x;
    if (y.type == LVAL_ERR) return y;
    if (x.type != y.type) {
        if (x.type == LVAL_LNG)
            x = lval_dbl(x.val.lng);
        if (y.type == LVAL_LNG)
            y = lval_dbl(y.val.lng);
    }

    if (x.type == LVAL_DBL) {
        if (strcmp(op, "+") == 0) return lval_dbl(x.val.dbl + y.val.dbl);
        else if (strcmp(op, "-") == 0) return lval_dbl(x.val.dbl - y.val.dbl);
        else if (strcmp(op, "*") == 0) return lval_dbl(x.val.dbl * y.val.dbl);
        else if (strcmp(op, "/") == 0) return lval_dbl(x.val.dbl / y.val.dbl);
        else if (strcmp(op, "^") == 0) return lval_dbl(pow(x.val.dbl, y.val.dbl));
        else if (strcmp(op, "min") == 0) {
            if (x.val.dbl > y.val.dbl) return y;
            else return x;
        }
        else if (strcmp(op, "max") == 0) {
            if (x.val.dbl > y.val.dbl) return x;
            else return y;
        }
        else if (strcmp(op, "and") == 0) {
            if (x.val.dbl == 0) return x;
            else return y;
        }
        else if (strcmp(op, "or") == 0) {
            if (x.val.dbl != 0) return x;
            else return y;
        }
        else if (strcmp(op, "%") == 0) return lval_err(LERR_BAD_TYPE);
        else return lval_err(LERR_BAD_OP);
    } else if (x.type == LVAL_LNG) {
        if (strcmp(op, "+") == 0) return lval_lng(x.val.lng + y.val.lng);
        else if (strcmp(op, "-") == 0) return lval_lng(x.val.lng - y.val.lng);
        else if (strcmp(op, "*") == 0) return lval_lng(x.val.lng * y.val.lng);
        else if (strcmp(op, "/") == 0) return lval_lng(x.val.lng / y.val.lng);
        else if (strcmp(op, "^") == 0) return lval_lng(pow(x.val.lng, y.val.lng));
        else if (strcmp(op, "min") == 0) {
            if (x.val.lng > y.val.lng) return y;
            else return x;
        }
        else if (strcmp(op, "max") == 0) {
            if (x.val.lng > y.val.lng) return x;
            else return y;
        }
        else if (strcmp(op, "and") == 0) {
            if (x.val.lng == 0) return x;
            else return y;
        }
        else if (strcmp(op, "or") == 0) {
            if (x.val.lng != 0) return x;
            else return y;
        }
        else if (strcmp(op, "%") == 0) return lval_lng(x.val.lng % y.val.lng);
        else return lval_err(LERR_BAD_OP);
    } else {
        return lval_err(LERR_BAD_TYPE);
    }
}

lval eval_ast(mpc_ast_t* ast) {

    if (strstr(ast->tag, "number")) {
        errno = 0;
        if (strstr(ast->contents, ".")){
            double x = strtod(ast->contents, NULL);
            return errno != ERANGE ? lval_dbl(x) : lval_err(LERR_BAD_NUM);
        } else {
            long x = strtol(ast->contents, NULL, 10);
            return errno != ERANGE ? lval_lng(x) : lval_err(LERR_BAD_NUM);
        }
    }
    
    if (strstr(ast->tag, "expr")) {
        lval x;
        int assoc;
        char* op = ast->children[1]->contents;

        if (strcmp(op, "^") == 0) assoc = ASSOC_RIGHT;
        else assoc = ASSOC_LEFT;

        if (assoc == ASSOC_LEFT) {
            x = eval_ast(ast->children[2]);
            for (int i=3; i < (ast->children_num - 1); i++)
                x = eval_op(op, x, eval_ast(ast->children[i]));
        } else {
            x = eval_ast(ast->children[(ast->children_num - 2)]);
            for (int i=(ast->children_num - 3); i > 1; i--)
                x = eval_op(op, eval_ast(ast->children[i]), x);
        }
        if (strcmp(op, "-") == 0 && ast->children_num == 4)
            x = eval_op("-", lval_lng(0), x);
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
            number      : /[0-9]*[.][0-9]+/ | /[0-9]+[.]?/ ;                  \
            operator    : '+' | '-' | '/' | '*' | '^' | \"min\" | \"max\" |   \
                          \"and\" | \"or\" | '%' ;                            \
            expr        : <number> | <decimal> |'(' <operator> <expr>+ ')' ;  \
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
            // mpc_ast_print(a);
            println_lval(eval_ast(a));
            mpc_ast_delete(res.output);
        } else {
            mpc_err_print(res.error);
            mpc_err_delete(res.error);
        }
        free(input);
    }
    return 0;
}
