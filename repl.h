#ifndef repl_h
# define repl_h

#include "mpc.h"

// Lisp Value (lval) type
typedef struct lval {
    enum { LVAL_LNG, LVAL_DBL, LVAL_ERR, LVAL_SYM, LVAL_SEXPR, LVAL_QEXPR } type;
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

lval* lval_dbl(double x);
lval* lval_lng(long x);
lval* lval_err(char* m);
lval* lval_sexpr(void);
lval* lval_qexpr(void);
lval* lval_sym(char* s);
void lval_del(lval* v);
lval* lval_add(lval* p, lval* c);
lval* lval_read_num(mpc_ast_t* ast);
lval* lval_read(mpc_ast_t* ast);
void lval_print_expr(lval* v, char open, char close);
void lval_print(lval* v);
void lval_println(lval* v);
lval* lval_pop(lval* v, int n);
lval* lval_take(lval* v, int n);
lval* lval_to_dbl(lval* v);
lval* builtin_op_dbl(lval* x, lval* y, char* op);
lval* builtin_op_lng(lval* x, lval* y, char* op);
lval* builtin_op(lval* a, char* op);
lval* builtin_head(lval* a);
lval* builtin_tail(lval* a);
lval* builtin_list(lval* a);
lval* builtin_eval(lval* a);
lval* builtin_join(lval* a);
lval* builtin(lval* a, char* func);
lval* lval_eval_sexpr(lval* v);
lval* lval_eval(lval* v);
int main(int argc, char** argv);
#endif
