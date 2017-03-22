#ifndef repl_h
# define repl_h

#include "mpc.h"

struct lval;
typedef struct lval lval;
struct lenv;
typedef struct len lenv;
typedef lval *(*lbuiltin)(lenv*, lval*);

lval* lval_dbl(double);
lval* lval_lng(long);
lval* lval_err(char*);
lval* lval_sexpr(void);
lval* lval_qexpr(void);
lval* lval_sym(char*);
lval* lval_proc(lbuiltin);

lval* lval_add(lval*, lval*);
lval* lval_pop(lval*, int);
lval* lval_insert(lval*, lval*, int);
lval* lval_take(lval*, int);
lval* lval_copy(lval*);
void lval_del(lval*);
lval* lval_to_dbl(lval*);

void lval_print(lval*);
void lval_println(lval*);
void lval_print_expr(lval*, char, char);

lval* builtin_op(lval*, char*);
lval* builtin_op_dbl(lval*, lval*, char*);
lval* builtin_op_lng(lval*, lval*, char*);
lval* builtin_list(lval*);
lval* builtin_eval(lval*);
lval* builtin_join(lval*);
lval* builtin_cons(lval*);
lval* builtin_len(lval*);
lval* builtin_car(lval*);
lval* builtin_cdr(lval*);
lval* builtin_nth(lval*);
lval* builtin_min(lval*);
lval* builtin_max(lval*);
lval* builtin_init(lval*);
lval* builtin_last(lval*);
lval* builtin_c__r(lval*, char*);
lval* builtin(lval*, char*);

lval* lval_read(mpc_ast_t*);
lval* lval_read_num(mpc_ast_t*);
lval* lval_eval(lval*);
lval* lval_eval_sexpr(lval*);

void exec_line(char*);
void exec_file(char*);
int main(int, char**);

#endif
