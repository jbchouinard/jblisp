# jblisp

A small dialect of Lisp.

## Build

```bash
./build.sh
```

## Run tests

```bash
./test.sh
```

## Design Notes

### Quoted expressions

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

### Error handling

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

### Closures / lexical scoping

We want bindings to happen within the innermost scope.

The only built-in construct in the language that creates a new scope is the
lambda. (the 'fun' procedure is not builtin, it is syntactic sugar on top
of a lambda expression).

A lambda value always has at least a frame containing it's formal parameters.

Should this frame be created when the lambda is called or when it is created?

I think a new env should be created then lambda is called. But the difficulty
is that the lambda must save a ref to its closure at the time it is defined.

Let's look at a simple case:

(def {z} 200)

(def {f} (\ {x} {(\ {y} {(+ x y z)})}))

(f 4) should return a lambda value (procedure), attached to a closure where
x=4.

((f 4) 10) should return 214.

BUT:

(def {g} (\ {z} {((f 4) 10)}))

What should (g 20) return? 34 or 214?

In Scheme the result is 214. On reflection it makes sense. That's the point
of lexical scoping; we take the binding z close to where the procedure is
declared.

It's worth noting we want to take the *binding* of z where it is defined, not
its *value*. If the value of that particular *binding* of z (in that particular
env) changes later, we take the new value.

Here is another tricky one:

(def {f} (\ {} {
    (def {z} 100)
    (def {g} (\ {} {(* 2 z)}))
    (def {z} 200)
    g
    })

((f)) should return 400, not 200.

For all this to be true, the following must happen:

- At the time of definition (creation of the lambda value, type LVAL_PROC),
  a new empty env is created and attached to the lambda value, with the current
  exec env as parent (which must be the env frame of the innermost scope, which
  must be either the global env, or a closure).
- When the lambda value is called, a copy of it's env is taken. The formal
  parameters are evaluated within the current exec env, and bound in the
  copy of the lambda's env.
- If a lambda is defined within the body of that lambda, it will have a new
  empty env, that will point to the copy of the parent lambda's env.
- The env attached to a lambda will never contain values.
- In that case it makes sense to only save a ref to the exec env, and create
  a env with it as parent instead of repeatedly taking copies of an empty env
  (it's a fairly minor difference but makes thinks conceptually easier - it
  makes it clear lambda values don't carry around a personal env with state,
  but rather a ref to a closure).

## TODO

### Extend standard library

complex arithmetic
rational arithmetic
filter, for-each
string manipulation
os, file input/output facilities

### Error handling facilities

Catch errors of type `type`:
(catch type {(...) (...) ...} callback)

Inspect error values:
(err-type err)
(err-msg err)
(err-values err)

Raise an error of type `type`:
(raise type msg v1 v2 ...)

Raise an error iff evaluating `predicate` does *not* raise an error of type `type`.
(assert-error type  msg v1 v2 ...)

### Macros

Macros let us stop the evaluation of arguments and do something with them.

They would let us implement a cleaner syntax for definitions, among other
things.

### Reduce memory footprint

We have lexical scoping with indefinite extent. Currently we keep all
closures (LENVs) in memory forever. This is not sustainable.

Each procedure call creates a new closure. If a lambda is defined in the call,
then that lambda will hold a ref to the created closure, so we must keep it,
but most calls do not define a lambda; the closure can be safely deleted.

Additionally we never reuse any LVALs. We always take copies. Thus, LVALs are
practically immutable. Everything is pass-by-value, even for large data
structures. Code is responsible for deleting LVALs that they create (valgrind
indicates there are no memory leak, so currently the code is doing that
correctly.)

#### Garbage collection of closures (LENVs)

As a first step we could keep immutable LVALs and implement refcount gc for
the LENVs. It should make memory use a bit less egregious. With immutable
LVALs, when an LENV is deleted we can simply delete all the LVALs it holds,
since currently lenv\_put takes a copy of the LVAL.

LVALs that hold references to LENVs must incr/decr the refcount on those LENVs
when appropriate.

#### Mutability and garbage collection of LVALs

lenv\_put and lenv\_get must be modified to take refs to LVALs instead of taking
copies. Once that is done, builtins must no longer delete LVALs that they put
in an env.

LVALs that hold refenrences to other LVALs must incr/decr the refcount on those
LVALs at appropriate times.

LENVs must incr/decr the refcount on LVALs they point to.

### Hashtable environments

Currently LENVs are implemented as simple arrays with O(n) lookup
and O(n) insert.
