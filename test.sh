echo "RUNNING C TESTS..."
echo ""
./bin/test
echo ""
echo ""
echo "RUNNING LISP TESTS..."
echo ""
./bin/jblisp --stop tests/test.jbl
