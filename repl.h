#ifndef repl_h
# define repl_h

#include "mpc.h"

struct lval;
typedef struct lval lval;
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
lval* builtin_list(lval* a);
lval* builtin_eval(lval* a);
lval* builtin_join(lval* a);
lval* builtin_cons(lval* a);
lval* builtin_car(lval* a);
lval* builtin_cdr(lval* a);
lval* builtin_init(lval* a);
lval* builtin_last(lval* a);
lval* builtin_c__r(lval* a, char* func);
lval* builtin(lval* a, char* func);
lval* lval_eval_sexpr(lval* v);
lval* lval_eval(lval* v);
int main(int argc, char** argv);
#endif
