# Design Notes

## Quoted expressions

Currently JBLisp does not have Macros, in particular it does not have a quote
macro ' to stop evaluation of symbolic expressions.

Instead JBLisp has a different syntax for expressions that are not evaluated
(or, more accurately, the expression evaluates to itself), the quoted
expression, represented by a list in braces.

So where you'd use a quoted symbol, say 'x, in most Lisps, you'll be using
a quoted symbol like {x}. Blocks of expressions for function definitions are
written as a list of symbolic expressions inside a quoted expression, like:

```lisp
(fun {f x y}
     {(+ x y)
      (- x y)})
```

This syntax is a bit heavier but it will have to do until (unless) we implement
macros.

## Error handling

We want to be able to catch errors.

Currently `lval_eval_sexpr` evaluates arguments first before calling the procedure,
and returns early if any of the args evaluates to an error.

The simplest way to add error handling to the language is probably to use
quoted expressions, like:

Catch errors of type `types`:
(catch type {(...) (...) ...} callback)

Where callback should be a procedure that takes 1 argument of type error.
We also need to add facilities to inspect and handle error values because
right now there is nothing useful for the callback to do.

At the very least we need to be able to inspect the error type, message and
extra values (v1, v2, ...).

(err-type err)
(err-msg err)
(err-values err)

We can add procedures to raise errors:

Raise an error of type `type`:
(raise type msg v1 v2 ...)

Raise an error iff evaluating `predicate` does *not* raise an error of type `type`.
(assert-error type  msg v1 v2 ...)
