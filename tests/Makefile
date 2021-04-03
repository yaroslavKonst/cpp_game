.PHONY: tests %_test

tests: connection_test

connection_test: connection_test.cpp
	g++ -Wall -c ../src/common/connection/connection.cpp\
		-o ../build/connection.o
	g++ -Wall -fopenmp -o ../build/$@ $< ../build/connection.o -lgtest
	../build/$@
