#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include "sudoku.h"

int64_t now() { // 返回时间
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000000 + tv.tv_usec;
}

int main(int argc, char *argv[]) {
  init_neighbors(); // 记录邻点位置neighbors[81][20]数组

  FILE *fp = fopen(argv[1], "r"); // 以只读模式打开文件
  char puzzle[128];
  int total_solved = 0;
  int total = 0;

  // 默认是solve_sudoku_basic函数，根据命令行参数调整使用的函数
  bool (*solve)(int) = solve_sudoku_basic;
  if (argv[2] != NULL) {
    if (argv[2][0] == 'a')
      solve = solve_sudoku_min_arity;
    else if (argv[2][0] == 'c')
      solve = solve_sudoku_min_arity_cache;
    else if (argv[2][0] == 'd')
      solve = solve_sudoku_dancing_links;
  }

  int64_t start = now();
  while (fgets(puzzle, sizeof puzzle, fp) != NULL) { // 读取文件流内容到puzzle中
    if (strlen(puzzle) >= N) {
      ++total;
      input(puzzle); // 输入到board数组中
      init_cache();  // occupied[81][10]数组，arity[81]数组
      if (solve(0)) {
        ++total_solved;
        if (!solved())
          assert(0);
      } else {
        printf("No: %s", puzzle);
      }
    }
  }
  int64_t end = now();
  double sec = (end - start) / 1000000.0;
  printf("%f sec %f ms each %d\n", sec, 1000 * sec / total, total_solved);

  return 0;
}
