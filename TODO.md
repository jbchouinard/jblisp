# TODO

## Implement error handling facilities

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
