#!/bin/bash
set -eu
echo "RUNNING C TESTS..."
./bin/test
echo "RUNNING LISP TESTS..."
./bin/jblisp --stop tests/test.jbl
