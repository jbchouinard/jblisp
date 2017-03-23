#ifndef repl_h
# define repl_h

#include "mpc.h"

#define VERSION "0.3.0"

extern mpc_parser_t *Boolean;
extern mpc_parser_t *Number;
extern mpc_parser_t *Symbol;
extern mpc_parser_t *Sexpr;
extern mpc_parser_t *Qexpr;
extern mpc_parser_t *Expr;
extern mpc_parser_t *JBLisp;

typedef struct _lval lval;
typedef struct _lenv lenv;
typedef lval *(*lbuiltin)(lenv*, lval*);

lenv *lenv_new(void);
lval *lenv_get(lenv*, char*);
lval *lenv_pop(lenv*, char*);
void lenv_del(lenv*);
void lenv_put(lenv*, char*, lval*);

lval* lval_bool(int);
lval* lval_dbl(double);
lval* lval_lng(long);
lval* lval_err(char*);
lval* lval_sexpr(void);
lval* lval_qexpr(void);
lval* lval_sym(char*);
lval* lval_proc(lbuiltin);
lval* lval_to_dbl(lval*);

lval* lval_add(lval*, lval*);
lval* lval_pop(lval*, int);
lval* lval_insert(lval*, lval*, int);
lval* lval_take(lval*, int);
lval* lval_copy(lval*);
void lval_del(lval*);
int lval_equal(lval*, lval*);
int lval_is(lval*, lval*);
int lval_is_true(lval*);

void lval_print(lval*);
void lval_println(lval*);
void lval_print_expr(lval*, char, char);

lval* builtin_arith(lval*, char*);
lval* builtin_arith_dbl(lval*, lval*, char*);
lval* builtin_arith_lng(lval*, lval*, char*);

lval* builtin_list(lenv*, lval*);
lval* builtin_eval(lenv*, lval*);
lval* builtin_join(lenv*, lval*);
lval* builtin_cons(lenv*, lval*);
lval* builtin_len(lenv*, lval*);
lval* builtin_head(lenv*, lval*);
lval* builtin_tail(lenv*, lval*);
lval* builtin_init(lenv*, lval*);
lval* builtin_last(lenv*, lval*);
lval* builtin_nth(lenv*, lval*);
lval* builtin_add(lenv*, lval*);
lval* builtin_sub(lenv*, lval*);
lval* builtin_mul(lenv*, lval*);
lval* builtin_div(lenv*, lval*);
lval* builtin_mod(lenv*, lval*);
lval* builtin_exp(lenv*, lval*);
lval* builtin_min(lenv*, lval*);
lval* builtin_max(lenv*, lval*);
lval* builtin_equal(lenv*, lval*);
lval* builtin_is(lenv*, lval*);
lval* builtin_and(lenv*, lval*);
lval* builtin_or(lenv*, lval*);
lval* builtin_not(lenv*, lval*);

lval* lval_read(mpc_ast_t*);
lval* lval_read_num(mpc_ast_t*);
lval* lval_eval(lenv*, lval*);
lval* lval_eval_sexpr(lenv*, lval*);

void exec_line(lenv*, char*);
void exec_file(lenv*, char*);
void build_parser(void);
void add_builtins(lenv*);

#endif
