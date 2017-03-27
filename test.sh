echo "RUNNING C TESTS..."
echo ""
./build/test
echo ""
echo ""
echo "RUNNING LISP TESTS..."
echo ""
./build/jblisp --stop tests/test.jbl
