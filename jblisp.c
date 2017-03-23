#include <string.h>
#include <math.h>

#include "mpc.h"
#include "jblisp.h"

mpc_parser_t *Boolean;
mpc_parser_t *Number;
mpc_parser_t *Symbol;
mpc_parser_t *Sexpr;
mpc_parser_t *Qexpr;
mpc_parser_t *Expr;
mpc_parser_t *JBLisp;

#define LASSERT(args, cond, err) \
    if (!(cond)) { lval_del(args); return lval_err(err); }

#define LASSERT_ARGC(fname, args, argc)                        \
    if (args->count != argc) {                                 \
        lval *err = lval_err(                                  \
            "Procedure '%s' expected %i argument(s), got %i.", \
            fname, argc, args->count);                         \
        lval_del(args);                                        \
        return err;                                            \
    }

#define LASSERT_ARGT(fname, args, idx, exp_type)                           \
    if (args->val.cell[idx]->type != exp_type) {                           \
        lval* err = lval_err(                                              \
            "Procedure '%s' expected argument %i of type '%s', got '%s'.", \
            fname, idx, TYPE_NAMES[exp_type],                              \
            TYPE_NAMES[args->val.cell[idx]->type]);                        \
        lval_del(args);                                                    \
        return err;                                                        \
    }

#define BAIL_IF_ERR(args)                          \
    for (int i=0; i < args->count; i++) {          \
        if (args->val.cell[i]->type == LVAL_ERR) { \
            return lval_take(args, i);             \
        }                                          \
    }

// Lisp types
enum { LVAL_BOOL, LVAL_LNG, LVAL_DBL, LVAL_ERR, LVAL_SYM,
       LVAL_BUILTIN, LVAL_LAMBDA, LVAL_SEXPR, LVAL_QEXPR };
char* TYPE_NAMES[] = {
    "boolean", "integer", "float", "error", "symbol",
    "builtin procedure", "lambda procedure", "S-expression", "Q-expression"
};

struct _lval {
    int type;
    int count;
    int size;
    union {
        enum {LFALSE=0, LTRUE=!LFALSE} bool;
        double dbl;
        long lng;
        char *err;
        char *sym;
        lval **cell;
        lbuiltin proc;
    } val;
};

struct _lenv {
    int count;
    int size;
    lenv *encl; // enclosing environment
    char **syms;
    lval **vals;
};

lenv *lenv_new(void) {
    lenv *e = malloc(sizeof(lenv));
    e->count = 0;
    e->size = 0;
    e->encl = NULL;
    e->syms = NULL;
    e->vals = NULL;
    return e;
}

void lenv_del(lenv *e) {
    for (int i=0; i < e->count; i++) {
        free(e->syms[i]);
        lval_del(e->vals[i]);
    }
    free(e->syms);
    free(e->vals);
    free(e);
}

void lenv_put(lenv *e, char *sym, lval *v) {
    for (int i=0; i < e->count; i++) {
        if (strcmp(e->syms[i], sym) == 0) {
            lval_del(e->vals[i]);
            e->vals[i] = lval_copy(v);
            return;
        }
    }
    // Symbol not found in env, append it
    e->count++;
    if (e->count > e->size) {
        e->size = e->size ? e->size * 2 : e->count;
        e->syms = realloc(e->syms, sizeof(char*) * e->size);
        e->vals = realloc(e->vals, sizeof(lval*) * e->size);
    }
    e->syms[e->count-1] = malloc(strlen(sym) + 1);
    strcpy(e->syms[e->count-1], sym);
    e->vals[e->count-1] = lval_copy(v);
}

lval *lenv_get(lenv *e, char *sym) {
    for (int i=0; i < e->count; i++) {
        if (strcmp(e->syms[i], sym) == 0) {
            return lval_copy(e->vals[i]);
        }
    }
    if (e->encl != NULL) {
        return lenv_get(e->encl, sym);
    }
    return lval_err("Unbound symbol '%s'.", sym);
}

lval *lval_bool(int b) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_BOOL;
    v->val.bool = b ? LTRUE : LFALSE;
    v->count = 0;
    v->size = 0;
    return v;
}

lval *lval_dbl(double x) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_DBL;
    v->val.dbl = x;
    v->count = 0;
    v->size = 0;
    return v;
}

lval *lval_lng(long x) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_LNG;
    v->val.lng = x;
    v->count = 0;
    v->size = 0;
    return v;
}

lval *lval_err(char *fmt, ...) {
    va_list va;
    va_start(va, fmt);

    lval *v = malloc(sizeof(lval));
    v->type = LVAL_ERR;
    v->count = 0;
    v->size = 0;
    v->val.err = malloc(512);
    vsnprintf(v->val.err, 511, fmt, va);
    v->val.err = realloc(v->val.err, strlen(v->val.err)+1);

    va_end(va);
    return v;
}

lval *lval_sexpr(void) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_SEXPR;
    v->val.cell = NULL;
    v->count = 0;
    v->size = 0;
    return v;
}

lval *lval_qexpr(void) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_QEXPR;
    v->val.cell = NULL;
    v->count = 0;
    v->size = 0;
    return v;
}

lval *lval_sym(char *s) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_SYM;
    v->count = 0;
    v->size = 0;
    v->val.sym = malloc(sizeof(strlen(s) + 1));
    strcpy(v->val.sym, s);
    return v;
}

lval *lval_proc(lbuiltin proc) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_BUILTIN;
    v->count = 0;
    v->size = 0;
    v->val.proc = proc;
    return v;
}

lval *lval_lambda(void) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_LAMBDA;
    v->val.cell = NULL;
    v->count = 0;
    v->size = 0;
    return v;
}

int lval_is(lval *v, lval *w) {
    if (v->type == w->type) {
        return v == w;
    } else {
        return 0;
    }
}

int lval_equal(lval *v, lval *w) {
    int eq = 0;
    if (v->type != w->type) {
        return eq;
    }
    switch(v->type) {
        case LVAL_BOOL:
            eq = v->val.bool == w->val.bool;
            break;
        case LVAL_DBL:
            eq = v->val.dbl == w->val.dbl;
            break;
        case LVAL_LNG:
            eq = v->val.lng == w->val.lng;
            break;
        case LVAL_ERR:
            eq = v == w;
            break;
        case LVAL_SYM:
            eq = !(strcmp(v->val.sym, w->val.sym));
            break;
        case LVAL_BUILTIN:
            eq = v->val.proc == w->val.proc;
            break;
        case LVAL_SEXPR:
        case LVAL_QEXPR:
        case LVAL_LAMBDA:
            if (v->count == w->count) {
                eq = 1;
                for (int i=0; i < v->count; i++) {
                    if (!lval_equal(v->val.cell[i], w->val.cell[i])) {
                        eq = 0;
                        break;
                    }
                }
            }
            break;
    }
    return eq;
}

int lval_is_true(lval *v) {
    int t;
    switch(v->type) {
        case LVAL_BOOL:
            t = v->val.bool;
            break;
        default:
            t = LTRUE;
            break;
    }
    return t;
}

void lval_del(lval *v) {
    switch(v->type) {
        case LVAL_BOOL:
        case LVAL_DBL:
        case LVAL_LNG:
        case LVAL_BUILTIN:
            break;
        case LVAL_ERR:
            free(v->val.err);
            break;
        case LVAL_SYM:
            free(v->val.sym);
            break;
        case LVAL_SEXPR:
        case LVAL_QEXPR:
        case LVAL_LAMBDA:
            for (int i=0; i < v->count; i++)
                lval_del(v->val.cell[i]);
            free(v->val.cell);
            break;
    }
    free(v);
}

lval *lval_copy(lval *v) {
    lval *x = malloc(sizeof(lval));
    x->type = v->type;

    switch (v->type) {
        case LVAL_BOOL:
            x->val.bool = v->val.bool;
            break;
        case LVAL_DBL:
            x->val.dbl = v->val.dbl;
            break;
        case LVAL_LNG:
            x->val.lng = v->val.lng;
            break;
        case LVAL_ERR:
            x->val.err = malloc(strlen(v->val.err) + 1);
            strcpy(x->val.err, v->val.err);
            break;
        case LVAL_SYM:
            x->val.sym = malloc(strlen(v->val.sym) + 1);
            strcpy(x->val.sym, v->val.sym);
            break;
        case LVAL_BUILTIN:
            x->val.proc = v->val.proc;
            break;
        case LVAL_SEXPR:
        case LVAL_QEXPR:
        case LVAL_LAMBDA:
            x->count = v->count;
            x->val.cell = malloc(sizeof(lval*) * x->count);
            for (int i=0; i < v->count; i++) {
                x->val.cell[i] = lval_copy(v->val.cell[i]);
            }
            break;
    }
    return x;
}

lval *lval_insert(lval *x, lval *v, int n) {
    LASSERT(x, n <= x->count,
        "Array bounds error in INSERT.");
    x->count++;
    if (x->count >= x->size) {
        x->size = x->size == 0 ? x->count : x->size  *2;
        x->val.cell = realloc(x->val.cell, sizeof(lval*)  *x->size);
    }
    memmove(x->val.cell+n+1, x->val.cell+n, (x->count-n-1) * sizeof(lval*));
    x->val.cell[n] = v;
    return x;
}

lval *lval_add(lval *x, lval *v) {
    return lval_insert(x, v, x->count);
}

lval *lval_pop(lval *v, int n) {
    lval *res = v->val.cell[n];
    memmove(v->val.cell+n, v->val.cell+n+1, (v->count-n-1)  *sizeof(lval*));
    v->count--;
    return res;
}

// Take content of a cell, deleting the parent lval
lval *lval_take(lval *v, int n) {
    lval *res = lval_pop(v, n);
    lval_del(v);
    return res;
}

lval *lval_read_num(mpc_ast_t *ast) {
    errno = 0;
    if (strstr(ast->contents, ".") ||
        strstr(ast->contents, "e") ||
        strstr(ast->contents, "E"))
    {
        double x = strtod(ast->contents, NULL);
        if (errno == ERANGE) {
            return lval_err("Invalid number (float): %s.", ast->contents);
        }
        return lval_dbl(x);
    } else {
        long x = strtol(ast->contents, NULL, 10);
        if (errno == ERANGE) {
            return lval_err("Invalid number (integer): %s.", ast->contents);
        }
        return lval_lng(x);
    }
}

lval *lval_read(mpc_ast_t *ast) {
    if (strstr(ast->tag, "number"))
        return lval_read_num(ast);
    if (strstr(ast->tag, "symbol"))
        return lval_sym(ast->contents);
    if (strstr(ast->tag, "boolean")) {
        return lval_bool(
            (strcmp(ast->contents, "#t") == 0) ? LTRUE : LFALSE);
    }

    lval *x = NULL;
    if (strcmp(ast->tag, ">") == 0 || strstr(ast->tag, "sexpr"))
        x = lval_sexpr();
    else if (strstr(ast->tag, "qexpr"))
        x = lval_qexpr();
    else
        return lval_err(
            "Parser error: '%s' is not a valid type tag.",
            ast->tag);

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

void lval_print_expr(lval *v, char open, char close) {
    putchar(open);
    for (int i=0; i < v->count; i++) {
        lval_print(v->val.cell[i]);
        if (i != v->count-1)
            putchar(' ');
    }
    putchar(close);
}

void lval_print(lval *v) {
    switch (v->type) {
        case LVAL_BOOL:
            printf(v->val.bool ? "#t" : "#f");
            break;
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
            lval_print_expr(v, '(', ')');
            break;
        case LVAL_QEXPR:
            lval_print_expr(v, '{', '}');
            break;
        case LVAL_BUILTIN:
            printf("<builtin procedure at %p>", (void*) v->val.proc);
            break;
        case LVAL_LAMBDA:
            printf("<lambda procedure>");
            break;
        case LVAL_ERR:
            printf("Error: %s", v->val.err);
            break;
        default:
            printf("Error: LVAL has invalid type.");
            break;
    }
}

void lval_println(lval *v) {
    lval_print(v);
    putchar('\n');
}

lval *lval_to_dbl(lval *v) {
    switch (v->type) {
        case LVAL_BOOL:
            v->type = LVAL_DBL;
            v->val.dbl = v->val.bool ? 1.0 : 0.0;
            break;
        case LVAL_DBL:
            break;
        case LVAL_LNG:
            v->type = LVAL_DBL;
            v->val.dbl = (double) v->val.lng;
            break;
        default:
            lval_del(v);
            v = lval_err(
                "Type error: '%s' cannot be converted to number.",
                TYPE_NAMES[v->type]);
            break;
    }
    return v;
}

lval *builtin_arith_dbl(lval *x, lval *y, char *op) {
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
    else {
        lval_del(x);
        x = lval_err(
            "Bad operator '%s' for type '%s'",
            op, TYPE_NAMES[LVAL_DBL]);
    }
    lval_del(y);
    return x;
}

lval *builtin_arith_lng(lval *x, lval *y, char *op) {
    if (strcmp(op, "+") == 0) x->val.lng += y->val.lng;
    else if (strcmp(op, "-") == 0) x->val.lng -= y->val.lng;
    else if (strcmp(op, "*") == 0) x->val.lng *= y->val.lng;
    else if (strcmp(op, "/") == 0) x->val.lng /= y->val.lng;
    else if (strcmp(op, "^") == 0) x->val.lng = pow(y->val.lng, x->val.lng);
    else if (strcmp(op, "min") == 0) {
        if (x->val.lng > y->val.lng) x->val.lng = y->val.lng;
    }
    else if (strcmp(op, "max") == 0) {
        if (x->val.lng < y->val.lng) x->val.lng = y->val.lng;
    }
    else if (strcmp(op, "%") == 0) x->val.lng %= y->val.lng;
    else {
        lval_del(x);
        x = lval_err(
            "Bad operator '%s' for type '%s'",
            op, TYPE_NAMES[LVAL_LNG]);
    }
    lval_del(y);
    return x;
}

// Operator associativity types
enum { ASSOC_RIGHT, ASSOC_LEFT };

lval *builtin_arith(lval *a, char *op) {
    if (a->count == 0) {
        if (strcmp(op, "+") == 0) {
            lval_del(a);
            return lval_lng(0);
        } else if (strcmp(op, "*") == 0) {
            lval_del(a);
            return lval_lng(1);
        } else {
            lval_del(a);
            return lval_err("Builtin op '%s' takes at least 1 argument.", op);
        }
    }

    // If any operand is a double, cast all to double
    int type = LVAL_LNG;
    for (int i=0; i < a->count; i++) {
        if (a->val.cell[i]->type == LVAL_DBL)
            type = LVAL_DBL;
        else if (a->val.cell[i]->type != LVAL_LNG) {
            lval *err = lval_err(
                "Builtin op '%s' not applicable on type '%s'.",
                op, TYPE_NAMES[a->val.cell[i]->type]);
            lval_del(a);
            return err;
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

    lval *x;
    if (assoc == ASSOC_LEFT) x = lval_pop(a, 0);
    else                     x = lval_pop(a, a->count-1);

    // (- x) = -x
    if ((strcmp(op, "-") == 0) && a->count == 0) {
        if (type == LVAL_DBL) {
            lval_del(a);
            return builtin_arith_dbl(lval_dbl(0.0), x, op);
        } else {
            lval_del(a);
            return builtin_arith_lng(lval_lng(0), x , op);
        }
    }
    // (/ x) = 1 / x
    if ((strcmp(op, "/") == 0) && a->count == 0) {
        if (type == LVAL_DBL) {
            lval_del(a);
            return builtin_arith_dbl(lval_dbl(1.0), x, op);
        } else {
            lval_del(a);
            return builtin_arith_lng(lval_lng(1), x , op);
        }
    }

    while (a->count) {
        lval *y;
        if (assoc == ASSOC_LEFT) y = lval_pop(a, 0);
        else                     y = lval_pop(a, a->count-1);

        switch (type) {
            case LVAL_DBL:
                x = builtin_arith_dbl(x, y, op);
                break;
            case LVAL_LNG:
                x = builtin_arith_lng(x, y, op);
                break;
        }
        if (x->type == LVAL_ERR) break;
    }
    lval_del(a);
    return x;
}

lval* builtin_add(lenv *e, lval *a) {
    return builtin_arith(a, "+");
}

lval* builtin_sub(lenv *e, lval *a) {
    return builtin_arith(a, "-");
}

lval* builtin_mul(lenv *e, lval *a) {
    return builtin_arith(a, "*");
}

lval* builtin_div(lenv *e, lval *a) {
    return builtin_arith(a, "/");
}

lval* builtin_mod(lenv *e, lval *a) {
    return builtin_arith(a, "%");
}

lval* builtin_exp(lenv *e, lval *a) {
    return builtin_arith(a, "^");
}

lval* builtin_min(lenv *e, lval *a) {
    return builtin_arith(a, "min");
}

lval* builtin_max(lenv *e, lval *a) {
    return builtin_arith(a, "max");
}

lval *builtin_nth(lenv *e, lval *a) {
    LASSERT_ARGC("nth", a, 2)
    LASSERT_ARGT("nth", a, 0, LVAL_QEXPR);
    LASSERT_ARGT("nth", a, 1, LVAL_LNG);
    LASSERT(a, a->val.cell[0]->count != 0,
        "Procedure 'nth' undefined on empty list '{}'.");

    int n = (int) a->val.cell[1]->val.lng;
    if (n < 0)
        n = a->val.cell[0]->count + n;

    LASSERT(a, n >= 0 ,
        "Array bounds error at procedure 'nth'.");
    LASSERT(a, n < a->val.cell[0]->count,
        "Array bounds error at procedure 'nth'.");

    // Delete all but first cell
    lval *v = lval_take(a, 0);
    return lval_take(v, n);
}

lval *builtin_head(lenv *e, lval *a) {
    LASSERT_ARGC("head", a, 1);
    LASSERT_ARGT("head", a, 0, LVAL_QEXPR);
    LASSERT(a, a->val.cell[0]->count != 0,
        "Procedure 'head' undefined on empty list '{}'.");

    // Delete all but first cell
    lval *v = lval_take(a, 0);
    return lval_take(v, 0);
}

lval *builtin_tail(lenv *e, lval *a) {
    LASSERT_ARGC("tail", a, 1)
    LASSERT(a, a->val.cell[0]->type == LVAL_QEXPR,
        "Type Error - procedure 'tail' expected a Q expression.");
    LASSERT(a, a->val.cell[0]->count != 0,
        "Procedure 'tail' undefined on empty list '{}'.");

    //Delete head
    lval *v = lval_take(a, 0);
    lval_del(lval_pop(v, 0));
    return v;
}

lval *builtin_list(lenv *e, lval *a) {
    LASSERT(a, a->type == LVAL_SEXPR,
        "Procedure 'list' expected an S expression");
    a->type = LVAL_QEXPR;
    return a;
}

lval *builtin_eval(lenv *e, lval *a) {
    LASSERT_ARGC("eval", a, 1);
    LASSERT(a, a->val.cell[0]->type == LVAL_QEXPR,
        "Procedure 'eval' expected a Q expression");

    lval *x = lval_take(a, 0);
    x->type = LVAL_SEXPR;
    return lval_eval(e, x);
}

lval *builtin_join(lenv *e, lval *a) {
    LASSERT(a, a->count != 0,
        "Procedure 'join' takes at least 1 argument.");
    for (int i=0; i < a->count; i++) {
        LASSERT(a, a->val.cell[i]->type == LVAL_QEXPR,
            "Procedure 'join' takes only Q expression arguments.");
    }

    lval *x = lval_pop(a, 0);
    while (a->count) {
        lval *y = lval_pop(a, 0);
        while (y->count)
            x = lval_add(x, lval_pop(y, 0));
        lval_del(y);
    }
    lval_del(a);
    return x;
}

lval *builtin_cons(lenv *e, lval *a) {
    LASSERT_ARGC("cons", a, 2);
    LASSERT(a, a->val.cell[1]->type == LVAL_QEXPR,
        "Second argument to 'cons' must be a Q expression.");
    lval *x = lval_pop(a, 0);
    lval *q = lval_take(a, 0);
    q = lval_insert(q, x, 0);
    return q;
}

lval *builtin_len(lenv *e, lval *a) {
    LASSERT_ARGC("len", a, 1);
    LASSERT(a, a->val.cell[0]->type == LVAL_QEXPR,
        "Procedure 'len' only applies to Q expressions.");
    lval *x = lval_lng(a->val.cell[0]->count);
    lval_del(a);
    return x;
}

lval *builtin_init(lenv *e, lval *a) {
    LASSERT_ARGC("init", a, 1);
    LASSERT(a, a->val.cell[0]->type == LVAL_QEXPR,
        "Procedure 'init' only applies to Q expressions.");

    // Keep all but first cell
    lval *x = lval_take(a, 0);
    lval_del(lval_pop(x, x->count-1));
    return x;
}

lval *builtin_last(lenv* e, lval *a) {
    LASSERT_ARGC("last", a, 1);
    LASSERT(a, a->val.cell[0]->type == LVAL_QEXPR,
        "Procedure 'last' only applies to Q expressions.");

    lval *x = lval_take(a, 0);
    return lval_take(x, x->count-1);
}

lval *builtin_equal(lenv* e, lval *a) {
    LASSERT_ARGC("equal?", a, 2);

    int eq = lval_equal(a->val.cell[0], a->val.cell[1]);
    lval_del(a);
    return lval_lng((long) eq);
}

lval *builtin_is(lenv* e, lval *a) {
    LASSERT_ARGC("equal?", a, 2);

    lval *v = a->val.cell[1];
    lval *w = lval_take(a, 0);
    return lval_lng(lval_is(v, w));
}

lval *builtin_and(lenv* e, lval *a) {
    int i;
    for (i=0; i < a->count; i++) {
        if (!lval_is_true(a->val.cell[i])) {
            return lval_take(a, i);
        }
    }
    if (a->count == 0) {
        lval_del(a);
        return lval_bool(LTRUE);
    } else {
        return lval_take(a, --i);
    }
}

lval *builtin_or(lenv* e, lval *a) {
    int i;
    for (i=0; i < a->count; i++) {
        if (lval_is_true(a->val.cell[i])) {
            return lval_take(a, i);
        }
    }
    if (a->count == 0) {
        lval_del(a);
        return lval_bool(LFALSE);
    } else {
        return lval_take(a, --i);
    }
}

lval *builtin_not(lenv* e, lval *a) {
    LASSERT_ARGC("not", a, 1);
    lval* v = lval_take(a, 0);
    lval* b = lval_bool(lval_is_true(v) ? LFALSE : LTRUE);
    lval_del(v);
    return b;
}

lval *builtin_def(lenv* e, lval *a) {
    LASSERT(a, a->count > 1,
            "Builtin 'def' expected at least 2 arguments.");
    LASSERT_ARGT("def", a, 0, LVAL_QEXPR);

    lval *ks = lval_pop(a, 0);

    if (ks->count != a->count) {
        lval_del(a);
        lval_del(ks);
        return lval_err("Builtin 'def': wrong number of symbols.");
    }
    for (int i=0; i < ks->count; i++) {
        if (ks->val.cell[i]->type != LVAL_SYM) {
            lval_del(a);
            lval_del(ks);
            return lval_err("Builtin 'def': only symbols can be defined.");
        }
    }
    for (int i=0; i < ks->count; i++) {
        lenv_put(e, ks->val.cell[i]->val.sym, a->val.cell[i]);
    }
    lval_del(ks);
    return a;
}

lval *builtin_lambda(lenv* e, lval *a) {
    LASSERT_ARGC("lambda", a, 2);
    LASSERT_ARGT("lambda", a, 0, LVAL_QEXPR);
    LASSERT_ARGT("lambda", a, 1, LVAL_QEXPR);

    lval *syms = lval_pop(a, 0);
    for (int i=0; i < syms->count; i++) {
        if (syms->val.cell[i]->type != LVAL_SYM) {
            lval_del(a);
            lval_del(syms);
            return lval_err(
                "First argument to lambda must be list of symbols");
        }
    }
    lval *q = lval_take(a, 0);
    if (q->count == 0) {
        return lval_err("Invalid lambda expression - empty procedure body");
    }

    lval* v = lval_lambda();
    lval_add(v, syms);
    lval_add(v, q);
    return v;
}

void add_builtins(lenv *e) {
    lenv_put(e, "def", lval_proc(builtin_def));
    lenv_put(e, "equal?", lval_proc(builtin_equal));
    lenv_put(e, "is?", lval_proc(builtin_is));
    lenv_put(e, "\\", lval_proc(builtin_lambda));

    // List procedures
    lenv_put(e, "list", lval_proc(builtin_list));
    lenv_put(e, "eval", lval_proc(builtin_eval));
    lenv_put(e, "join", lval_proc(builtin_join));
    lenv_put(e, "cons", lval_proc(builtin_cons));
    lenv_put(e, "len", lval_proc(builtin_len));
    lenv_put(e, "head", lval_proc(builtin_head));
    lenv_put(e, "tail", lval_proc(builtin_tail));
    lenv_put(e, "init", lval_proc(builtin_init));
    lenv_put(e, "last", lval_proc(builtin_last));
    lenv_put(e, "nth", lval_proc(builtin_nth));

    // Arithmetic
    lenv_put(e, "+", lval_proc(builtin_add));
    lenv_put(e, "-", lval_proc(builtin_sub));
    lenv_put(e, "*", lval_proc(builtin_mul));
    lenv_put(e, "/", lval_proc(builtin_div));
    lenv_put(e, "%", lval_proc(builtin_mod));
    lenv_put(e, "^", lval_proc(builtin_exp));
    lenv_put(e, "min", lval_proc(builtin_min));
    lenv_put(e, "max", lval_proc(builtin_max));

    // Logic functions
    lenv_put(e, "and", lval_proc(builtin_and));
    lenv_put(e, "or", lval_proc(builtin_or));
    lenv_put(e, "not", lval_proc(builtin_not));
}

lval *lval_eval_sexpr(lenv *e, lval *v) {
    if (v->count == 0) {
        return v;
    }
    for (int i=0; i < v->count; i++) {
        v->val.cell[i] = lval_eval(e, v->val.cell[i]);
    }
    BAIL_IF_ERR(v);

    lval *proc = lval_pop(v, 0);

    if (proc->type == LVAL_BUILTIN) {
        lval *result = proc->val.proc(e, v);
        lval_del(proc);
        return result;
    } else if (proc->type == LVAL_LAMBDA) {
        // Set up lambda
        lval *syms = lval_pop(proc, 0);
        lval *lambda = lval_take(proc, 0);
        if (syms->count != v->count) {
            lval_del(syms);
            lval_del(lambda);
            lval_del(v);
            return lval_err("Wrong number of arguments to lambda.");
        }
        // Set up lambda env
        lenv *proc_env = lenv_new();
        proc_env->encl = e;
        for (int i=0; i < syms->count; i++) {
            lenv_put(proc_env, syms->val.cell[i]->val.sym, v->val.cell[i]);
        }
        // Evaluate
        lval *result;
        while (lambda->count) {
            result = lval_eval(proc_env, lval_pop(lambda, 0));
            if (result->type == LVAL_ERR) { break; }
        }
        // Cleanup
        lval_del(syms);
        lval_del(v);
        lenv_del(proc_env);
        return result;
    } else {
        lval *err = lval_err(
            "Object of type '%s' is not applicable.", TYPE_NAMES[proc->type]);
        lval_del(proc);
        lval_del(v);
        return err;
    }
}

lval *lval_eval(lenv *e, lval *v) {
    BAIL_IF_ERR(v);

    lval *x;
    switch (v->type) {
        case LVAL_SYM:
            x = lenv_get(e, v->val.sym);
            lval_del(v);
            break;
        case LVAL_SEXPR:
             x = lval_eval_sexpr(e, v);
             break;
        default:
            x = v;
            break;
    }
    return x;
}

void exec_file(lenv *e, char *filename) {
    mpc_result_t res;

    printf("Loading file '%s'...\n", filename);
    if (mpc_parse_contents(filename, JBLisp, &res)) {
        lval *prog = lval_read(res.output);
        mpc_ast_delete(res.output);
        while (prog->count) {
            lval *x = lval_eval(e, lval_pop(prog, 0));
            if (x->type == LVAL_ERR) {
                lval_println(x);
            }
            lval_del(x);
        }
        lval_del(prog);
    } else {
        mpc_err_print(res.error);
        mpc_err_delete(res.error);
    }
    puts("done.");
}

void exec_line(lenv *e, char *input) {
    mpc_result_t res;

    if (mpc_parse("<stdin>", input, JBLisp, &res)) {
        lval *line = lval_read(res.output);
        mpc_ast_delete(res.output);
        while (line->count) {
            lval *x = lval_eval(e, lval_pop(line, 0));
            lval_println(x);
            lval_del(x);
        }
        lval_del(line);
    } else {
        mpc_err_print(res.error);
        mpc_err_delete(res.error);
    }
}

void build_parser() {
    Boolean   = mpc_new("boolean");
    Number    = mpc_new("number");
    Symbol    = mpc_new("symbol");
    Sexpr     = mpc_new("sexpr");
    Qexpr     = mpc_new("qexpr");
    Expr      = mpc_new("expr");
    JBLisp    = mpc_new("jblisp");

    mpca_lang(MPCA_LANG_DEFAULT,
        "                                                                     \
            boolean  : /(#t)|(#f)/ ;                                     \
            number   : /((-?[0-9]*[.][0-9]+)|(-?[0-9]+[.]?))([eE]-?[0-9]+)?/ ;\
            symbol   : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&?\\^]+/ ;                 \
            sexpr    : '(' <expr>* ')' ;                                      \
            qexpr    : '{' <expr>* '}' ;                                      \
            expr     : <boolean > | <number> | <symbol> | <sexpr> | <qexpr> ; \
            jblisp   : /^/ <expr>* /$/ ;                                      \
        ",
        Boolean, Number, Symbol, Sexpr, Qexpr, Expr, JBLisp
    );
}

