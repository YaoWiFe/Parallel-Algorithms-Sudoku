CXXFLAGS+=-O2 -ggdb -DDEBUG
CXXFLAGS+=-Wall -Wextra

all: sudoku_solve

sudoku: main.cc neighbor.cc sudoku_basic.cc sudoku_min_arity.cc sudoku_min_arity_cache.cc sudoku_dancing_links.cc
	g++ -O2 -o $@ $^

sudoku_solve: sudoku_solve_threadpool.cc sudoku_dancing_links.cc
	g++ -O2 -o $@ $^ -pthread

sudoku_solve_basic: sudoku_solve.cc sudoku_dancing_links.cc
	g++ -O2 -o $@ $^ -pthread

clean: 
	rm sudoku_solve