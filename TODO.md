# TODO

## Extend standard library

complex arithmetic
rational arithmetic
filter, for-each
string manipulation
os, file input/output facilities

## Error handling facilities

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

## Macros

Macros let us stop the evaluation of arguments and do something with them.

They would let us implement a cleaner syntax for definitions, among other
things.

## Reduce memory footprint

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

### Garbage collection of closures (LENVs)

As a first step we could keep immutable LVALs and implement refcount gc for
the LENVs. It should make memory use a bit less egregious. With immutable
LVALs, when an LENV is deleted we can simply delete all the LVALs it holds,
since currently lenv\_put takes a copy of the LVAL.

LVALs that hold references to LENVs must incr/decr the refcount on those LENVs
when appropriate.

### Mutability and garbage collection of LVALs

lenv\_put and lenv\_get must be modified to take refs to LVALs instead of taking
copies. Once that is done, builtins must no longer delete LVALs that they put
in an env.

LVALs that hold refenrences to other LVALs must incr/decr the refcount on those
LVALs at appropriate times.

LENVs must incr/decr the refcount on LVALs they point to.

## Hashtable environments

Currently LENVs are implemented as simple arrays with O(n) lookup
and O(n) insert.
