#ifndef repl_h
# define repl_h

#include "mpc.h"

#define VERSION "0.6.0"

extern long COUNT_LVALCPY;
extern long COUNT_LVALNEW;
extern long COUNT_LVALDEL;
extern long COUNT_LPROCCPY;
extern long COUNT_LPROCNEW;
extern long COUNT_LPROCDEL;
extern long COUNT_LENVCPY;
extern long COUNT_LENVNEW;
extern long COUNT_LENVDEL;

typedef struct _lval lval;
typedef struct _lenv lenv;
typedef struct _lproc lproc;
typedef lval *(*lbuiltin)(lenv*, lval*);

extern lenv *LENVS[];

lenv *lenv_new(lenv*);
lval *lenv_get(lenv*, char*);
lval *lenv_pop(lenv*, char*);
void lenv_del(lenv*);
void lenv_put(lenv*, char*, lval*);

lproc *lproc_new(void);
lproc *lproc_copy(lproc*);
void lproc_del(lproc*);

lval *lval_bool(int);
lval *lval_dbl(double);
lval *lval_lng(long);
lval *lval_err(char*, ...);
lval *lval_sexpr(void);
lval *lval_qexpr(void);
lval *lval_sym(char*);
lval *lval_str(char*, int);
lval *lval_builtin(lbuiltin);
lval *lval_proc(void);

lval *lval_add(lval*, lval*);
lval *lval_pop(lval*, int);
lval *lval_insert(lval*, lval*, int);
lval *lval_take(lval*, int);
lval *lval_copy(lval*);
void lval_del(lval*);

int lval_equal(lval*, lval*);
int lval_is(lval*, lval*);
int lval_is_true(lval*);

lval *lval_repr(lval*);
void lval_print(lval*);
void lval_println(lval*);
void lval_print_expr(lval*, char, char);

lval *lval_arith(lval*, lval*, int);
lval *lval_lng_to_dbl(lval*);

lval *builtin_add(lenv*, lval*);
lval *builtin_sub(lenv*, lval*);
lval *builtin_mul(lenv*, lval*);
lval *builtin_div(lenv*, lval*);
lval *builtin_mod(lenv*, lval*);
lval *builtin_exp(lenv*, lval*);
lval *builtin_lt(lenv*, lval*);
lval *builtin_eq(lenv*, lval*);

lval *builtin_and(lenv*, lval*);
lval *builtin_or(lenv*, lval*);
lval *builtin_not(lenv*, lval*);

lval *builtin_def(lenv*, lval*);
lval *builtin_def_global(lenv*, lval*);
lval *builtin_lambda(lenv*, lval*);
lval *builtin_apply(lenv*, lval*);
lval *builtin_error(lenv*, lval*);
lval *builtin_assert(lenv*, lval*);
lval *builtin_equal(lenv*, lval*);
lval *builtin_is(lenv*, lval*);

lval *builtin_list(lenv*, lval*);
lval *builtin_eval(lenv*, lval*);
lval *builtin_join(lenv*, lval*);
lval *builtin_cons(lenv*, lval*);
lval *builtin_len(lenv*, lval*);
lval *builtin_head(lenv*, lval*);
lval *builtin_tail(lenv*, lval*);
lval *builtin_init(lenv*, lval*);
lval *builtin_last(lenv*, lval*);
lval *builtin_nth(lenv*, lval*);

lval *builtin_concat(lenv*, lval*);

lval *lval_read(mpc_ast_t*);
lval *lval_read_num(mpc_ast_t*);

lval *lval_eval(lenv*, lval*);
lval *lval_do(lenv*, lval*);
lval *lval_eval_sexpr(lenv*, lval*);
lval *lval_call(lenv*, lval*, lval*);

lval *load_file(lenv*, char*);
void exec_line(lenv*, char*);
void exec_file(lenv*, char*);
void build_parser(void);
void cleanup_parser(void);
void add_builtins(lenv*);

#endif
