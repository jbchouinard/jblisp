#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <editline/readline.h>
#include <editline/history.h>

#include "mpc.h"

typedef enum { LVAL_LNG, LVAL_DBL, LVAL_ERR, LVAL_SYM, LVAL_SEXPR } ltype;

// Lisp Value (lval) type
typedef struct lval {
    ltype type;
    int count;
    int size;
    union {
        double dbl;
        long lng;
        char* err;
        char* sym;
        struct lval** cell;
    } val;
} lval;

// Operator association types
enum { ASSOC_RIGHT, ASSOC_LEFT };

lval* lval_dbl(double x) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_DBL;
    v->val.dbl = x;
    return v;
}

lval* lval_lng(long x) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_LNG;
    v->val.lng = x;
    return v;
}

lval* lval_err(char* m) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_ERR;
    v->val.err = malloc(strlen(m) + 1);
    strcpy(v->val.err, m);
    return v;
}

lval* lval_sexpr(void) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SEXPR;
    v->val.cell = NULL;
    v->count = 0;
    v->size = 0;
    return v;
}

lval* lval_sym(char* s) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SYM;
    v->val.sym = malloc(sizeof(strlen(s) + 1));
    strcpy(v->val.sym, s);
    return v;
}

void lval_del(lval* v) {
    switch(v->type) {
        case LVAL_DBL:
        break;
        case LVAL_LNG:
        break;
        case LVAL_ERR:
            free(v->val.err);
        break;
        case LVAL_SYM:
            free(v->val.sym);
        break;
        case LVAL_SEXPR:
            for (int i=0; i < v->count; i++)
                lval_del(v->val.cell[i]);
            free(v->val.cell);
        break;
    }
    free(v);
}

lval* lval_add(lval* p, lval* c) {
    if (p->size == 0) {
        p->val.cell = malloc(sizeof(lval*));
        p->size = 1;
    }
    p->count++;
    if (p->count > p->size) {
        p->size = p->size * 2;
        p->val.cell = realloc(p->val.cell, sizeof(lval*) * p->size);
    }
    p->val.cell[p->count-1] = c;
    return p;
}

lval* lval_read_num(mpc_ast_t* ast) {
    errno = 0;
    if (strstr(ast->contents, ".")) {
        double x = strtod(ast->contents, NULL);
        return errno != ERANGE ? lval_dbl(x) : lval_err("invalid number");
    } else {
        long x = strtol(ast->contents, NULL, 10);
        return errno != ERANGE ? lval_lng(x) : lval_err("invalid number");
    }
}

lval* lval_read(mpc_ast_t* ast) {
    if (strstr(ast->tag, "number"))
        return lval_read_num(ast);
    if (strstr(ast->tag, "symbol"))
        return lval_sym(ast->contents);

    lval* x = NULL;
    if (strcmp(ast->tag, ">") == 0 || strstr(ast->tag, "sexpr"))
        x = lval_sexpr();
    else
        return lval_err("expected an S expression");

    for (int i=0; i < ast->children_num; i++) {
        if (strcmp(ast->children[i]->contents, "(") == 0)
            continue;
        if (strcmp(ast->children[i]->contents, ")") == 0)
            continue;
        if (strcmp(ast->children[i]->contents, "{") == 0)
            continue;
        if (strcmp(ast->children[i]->contents, "}") == 0)
            continue;
        if (strcmp(ast->children[i]->tag, "regex") == 0)
            continue;
        x = lval_add(x, lval_read(ast->children[i]));
    }
    return x;
}

void lval_print(lval* v) {
    switch (v->type) {
        case LVAL_LNG:
            printf("%li", v->val.lng);
            break;
        case LVAL_DBL:
            printf("%lf", v->val.dbl);
            break;
        case LVAL_SYM:
            printf("%s", v->val.sym);
            break;
        case LVAL_SEXPR:
            putchar('(');
            for (int i=0; i < v->count; i++) {
                lval_print(v->val.cell[i]);
                if (i != v->count-1)
                    putchar(' ');
            }
            putchar(')');
            break;
        case LVAL_ERR:
            printf("Error: %s", v->val.err);
            break;
        default:
            printf("Error: Lval has invalid type.");
            break;
    }
}

void lval_println(lval* v) {
    lval_print(v);
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

lval* eval_op(char* op, lval* x, lval* y) {
    if (x->type == LVAL_ERR) return x;
    if (y->type == LVAL_ERR) return y;
    if (x->type != y->type) {
        if (x->type == LVAL_LNG)
            x = lval_dbl(x->val.lng);
        if (y->type == LVAL_LNG)
            y = lval_dbl(y->val.lng);
    }

    if (x->type == LVAL_DBL) {
        if (strcmp(op, "+") == 0) return lval_dbl(x->val.dbl + y->val.dbl);
        else if (strcmp(op, "-") == 0) return lval_dbl(x->val.dbl - y->val.dbl);
        else if (strcmp(op, "*") == 0) return lval_dbl(x->val.dbl * y->val.dbl);
        else if (strcmp(op, "/") == 0) return lval_dbl(x->val.dbl / y->val.dbl);
        else if (strcmp(op, "^") == 0) return lval_dbl(pow(x->val.dbl, y->val.dbl));
        else if (strcmp(op, "min") == 0) {
            if (x->val.dbl > y->val.dbl) return y;
            else return x;
        }
        else if (strcmp(op, "max") == 0) {
            if (x->val.dbl > y->val.dbl) return x;
            else return y;
        }
        else if (strcmp(op, "and") == 0) {
            if (x->val.dbl == 0) return x;
            else return y;
        }
        else if (strcmp(op, "or") == 0) {
            if (x->val.dbl != 0) return x;
            else return y;
        }
        else if (strcmp(op, "%") == 0) return lval_err("bad type");
        else return lval_err("bad operator");
    } else if (x->type == LVAL_LNG) {
        if (strcmp(op, "+") == 0) return lval_lng(x->val.lng + y->val.lng);
        else if (strcmp(op, "-") == 0) return lval_lng(x->val.lng - y->val.lng);
        else if (strcmp(op, "*") == 0) return lval_lng(x->val.lng * y->val.lng);
        else if (strcmp(op, "/") == 0) return lval_lng(x->val.lng / y->val.lng);
        else if (strcmp(op, "^") == 0) return lval_lng(pow(x->val.lng, y->val.lng));
        else if (strcmp(op, "min") == 0) {
            if (x->val.lng > y->val.lng) return y;
            else return x;
        }
        else if (strcmp(op, "max") == 0) {
            if (x->val.lng > y->val.lng) return x;
            else return y;
        }
        else if (strcmp(op, "and") == 0) {
            if (x->val.lng == 0) return x;
            else return y;
        }
        else if (strcmp(op, "or") == 0) {
            if (x->val.lng != 0) return x;
            else return y;
        }
        else if (strcmp(op, "%") == 0) return lval_lng(x->val.lng % y->val.lng);
        else return lval_err("bad operator");
    } else {
        return lval_err("type error");
    }
}

lval* eval_ast(mpc_ast_t* ast) {

    if (strstr(ast->tag, "number")) {
        errno = 0;
        if (strstr(ast->contents, ".")){
            double x = strtod(ast->contents, NULL);
            return errno != ERANGE ? lval_dbl(x) : lval_err("invalid number");
        } else {
            long x = strtol(ast->contents, NULL, 10);
            return errno != ERANGE ? lval_lng(x) : lval_err("invalid number");
        }
    }
    
    if (strstr(ast->tag, "expr")) {
        lval* x;
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
    mpc_parser_t* Symbol    = mpc_new("symbol");
    mpc_parser_t* Sexpr     = mpc_new("sexpr");
    mpc_parser_t* Expr      = mpc_new("expr");
    mpc_parser_t* JBLisp    = mpc_new("jblisp");

    mpca_lang(MPCA_LANG_DEFAULT,
        "                                                                     \
            number      : /[0-9]*[.][0-9]+/ | /[0-9]+[.]?/ ;                  \
            symbol      : '+' | '-' | '/' | '*' | '^' | \"min\" | \"max\" |   \
                          \"and\" | \"or\" | '%' ;                            \
            sexpr       : '(' <expr>* ')' ;                                   \
            expr        : <number> | <symbol> | <sexpr> ;                     \
            jblisp      : /^/ <expr>* /$/ ;                                   \
        ",
        Number, Symbol, Sexpr, Expr, JBLisp
    );

    puts("jblisp version 0.1.0");
    puts("Press ^C to exit\n");

    mpc_result_t res;
    while (1) {
        char* input = readline("jblisp> ");
        // Parse input
        add_history(input);
        if (mpc_parse("<stdin>", input, JBLisp, &res)) {
            // mpc_ast_t* a = res.output;
            // mpc_ast_print(a);
            // println_lval(eval_ast(a));
            lval* x = lval_read(res.output);
            lval_println(x);
            lval_del(x);
            mpc_ast_delete(res.output);
        } else {
            mpc_err_print(res.error);
            mpc_err_delete(res.error);
        }
        free(input);
    }
    mpc_cleanup(5, Number, Symbol, Sexpr, Expr, JBLisp);
    return 0;
}
