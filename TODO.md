# TODO

## Builtins

set-head!
set-tail!
string?
number?
integer?
float?
boolean?

## Add more tests

### C Unit Tests

### Lisp Integration Tests

## Extend standard library

complex arithmetic
rational arithmetic
filter, for-each
string manipulation

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

## Garbage collection

We have lexical scoping with indefinite extent. Currently we keep all
closures in memory forever. This is not sustainable.

Additionally we never reuse any lvals. We always take copies.

## Hashtable environments

Currently environments are implemented as simple arrays with O(n) lookup
and O(n) insert.
