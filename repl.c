#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <editline/readline.h>
#include <editline/history.h>

#include "mpc.h"

// Lisp Value (lval) type
typedef struct lval {
    enum { LVAL_LNG, LVAL_DBL, LVAL_ERR, LVAL_SYM, LVAL_SEXPR } type;
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

// Operator associativity types
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
    p->count++;
    if (p->count >= p->size) {
        p->size = p->size == 0 ? 1 : p->size * 2;
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

// Pop content of a cell
lval* lval_pop(lval* v, int n) {
    lval* result = v->val.cell[n];
    for (int i=n+1; i < v->count; i++) {
        v->val.cell[i-1] = v->val.cell[i];
    }
    v->count--;
    v->val.cell[v->count] = NULL;
    return result;
}

// Take content of a cell, deleting the parent lval
lval* lval_take(lval* v, int n) {
    lval* result = NULL;
    for (int i=0; i < v->count; i++) {
        if (i == n)
            result = v->val.cell[i];
        else
            lval_del(v->val.cell[i]);
    }
    free(v->val.cell);
    free(v);
    return result;
}


lval* lval_eval_sexpr(lval*);
lval* builtin_op(lval*, char*);

lval* lval_eval(lval* v) {
    if (v->type == LVAL_SEXPR)
        return lval_eval_sexpr(v);
    else
        return v;
}

lval* lval_eval_sexpr(lval* v) {
    for (int i=0; i < v->count; i++)
        v->val.cell[i] = lval_eval(v->val.cell[i]);

    if (v->count == 0)
        return v;

    if (v->count == 1)
        return lval_take(v, 0);

    lval* op = lval_pop(v, 0);
    if (op->type != LVAL_SYM) {
        lval_del(op);
        lval_del(v);
        return lval_err("Excepted symbol at start of S expression.");
    }

    lval* result = builtin_op(v, op->val.sym);
    lval_del(op);
    return result;
}

lval* lval_to_dbl(lval* v) {
    switch (v->type) {
        case LVAL_DBL:
            break;
        case LVAL_LNG:
            v->type = LVAL_DBL;
            v->val.dbl = (double) v->val.lng;
            break;
        default:
            lval_del(v);
            v = lval_err("type error");
            break;
    }
    return v;
}

lval* builtin_op_dbl(lval* x, lval* y, char* op) {
    if (strcmp(op, "+") == 0) x->val.dbl += y->val.dbl;
    else if (strcmp(op, "-") == 0) x->val.dbl -= y->val.dbl;
    else if (strcmp(op, "*") == 0) x->val.dbl *= y->val.dbl;
    else if (strcmp(op, "/") == 0) x->val.dbl /= y->val.dbl;
    else if (strcmp(op, "^") == 0) x->val.dbl = pow(y->val.dbl, x->val.dbl);
    else if (strcmp(op, "min") == 0) {
        if (x->val.dbl > y->val.dbl) x->val.dbl = y->val.dbl;
    }
    else if (strcmp(op, "max") == 0) {
        if (x->val.dbl < y->val.dbl) x->val.dbl = y->val.dbl;
    }
    else if (strcmp(op, "and") == 0) {
        if (x->val.dbl != 0) x->val.dbl = y->val.dbl;
    }
    else if (strcmp(op, "or") == 0) {
        if (x->val.dbl == 0) x->val.dbl = y->val.dbl;
    }
    else {
        lval_del(x);
        x = lval_err("bad operator for type DOUBLE");
    }
    lval_del(y);
    return x;
}

lval* builtin_op_lng(lval* x, lval* y, char* op) {
    if (strcmp(op, "+") == 0) x->val.lng += y->val.lng;
    else if (strcmp(op, "-") == 0) x->val.lng -= y->val.lng;
    else if (strcmp(op, "*") == 0) x->val.lng *= y->val.lng;
    else if (strcmp(op, "/") == 0) x->val.lng /= y->val.lng;
    else if (strcmp(op, "^") == 0) x->val.lng = pow(y->val.lng, x->val.lng);
    else if (strcmp(op, "min") == 0) {
        if (x->val.dbl > y->val.dbl) x->val.dbl = y->val.dbl;
    }
    else if (strcmp(op, "max") == 0) {
        if (x->val.lng < y->val.lng) x->val.lng = y->val.lng;
    }
    else if (strcmp(op, "and") == 0) {
        if (x->val.lng != 0) x->val.lng = y->val.lng;
    }
    else if (strcmp(op, "or") == 0) {
        if (x->val.lng == 0) x->val.lng = y->val.lng;
    }
    else if (strcmp(op, "%") == 0) x->val.lng %= y->val.lng;
    else {
        lval_del(x);
        y = lval_err("bad operator for type LONG");
    }
    lval_del(y);
    return x;
}

lval* builtin_op(lval* a, char* op) {

    if (a->type == LVAL_ERR) return a;

    // If any operand is a double, cast all to double
    int type = LVAL_LNG;
    for (int i=0; i < a->count; i++) {
        if (a->val.cell[i]->type == LVAL_DBL)
            type = LVAL_DBL;
        else if (a->val.cell[i]->type != LVAL_LNG) {
            lval_del(a);
            return lval_err("cannot operate on non-number.");
        }
    }
    if (type == LVAL_DBL) {
        for (int i=0; i < a->count; i++) {
            a->val.cell[i] = lval_to_dbl(a->val.cell[i]);
        }
    }

    // Set associativity rule
    int assoc;
    if (strcmp(op, "^") == 0) assoc = ASSOC_RIGHT;
    else                      assoc = ASSOC_LEFT;

    lval* x;
    if (assoc == ASSOC_LEFT) x = lval_pop(a, 0);
    else                     x = lval_pop(a, a->count-1);

    // (- x) = -x
    if ((strcmp(op, "-") == 0) && a->count == 0) {
        switch (type) {
            case LVAL_DBL:
                x = builtin_op_dbl(lval_dbl(0.0), x, op);
                break;
            case LVAL_LNG:
                x = builtin_op_lng(lval_lng(0.0), x , op);
                break;
        }
    }

    while (a->count > 0) {
        lval* y;
        if (assoc == ASSOC_LEFT) y = lval_pop(a, 0);
        else                     y = lval_pop(a, a->count-1);

        switch (type) {
            case LVAL_DBL:
                x = builtin_op_dbl(x, y, op);
                break;
            case LVAL_LNG:
                x = builtin_op_lng(x, y, op);
                break;
        }
        if (x->type == LVAL_ERR) break;
    }
    lval_del(a);
    return x;
}

int main(int argc, char** argv) {
    mpc_parser_t* Number    = mpc_new("number");
    mpc_parser_t* Symbol    = mpc_new("symbol");
    mpc_parser_t* Sexpr     = mpc_new("sexpr");
    mpc_parser_t* Expr      = mpc_new("expr");
    mpc_parser_t* JBLisp    = mpc_new("jblisp");

    mpca_lang(MPCA_LANG_DEFAULT,
        "                                                                     \
            number      : /-?[0-9]*[.][0-9]+/ | /-?[0-9]+[.]?/ ;              \
            symbol      : '+' | '-' | '/' | '*' | '^' | \"min\" | \"max\" |   \
                          \"and\" | \"or\" | '%' ;                            \
            sexpr       : '(' <expr>* ')' ;                                   \
            expr        : <number> | <symbol> | <sexpr> ;                     \
            jblisp      : /^/ <expr>* /$/ ;                                   \
        ",
        Number, Symbol, Sexpr, Expr, JBLisp
    );

    puts("jblisp version 0.2.1");
    puts("Press ^C to exit\n");

    while (1) {
        char* input = readline("jblisp> ");
        add_history(input);

        mpc_result_t res;
        if (mpc_parse("<stdin>", input, JBLisp, &res)) {
            lval* x = lval_eval(lval_read(res.output));
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
