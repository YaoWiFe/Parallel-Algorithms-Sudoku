#include <assert.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <sys/time.h>
#include <thread>

#include "sudoku.h"
#include "threadpool.h"
using namespace std;

// ios和stdio的同步是否打开
#define SYNC 0
#define DEBUG 0
double total_time_start, total_time_end;
double time_start, time_end;
double sec;

ThreadPool thread_pool;
int thread_num = 1;
char one_puzzle[128]; // 用来存储单个数独题目的缓冲区
const int puzzle_buf_num = 10240;
int puzzle_buf[puzzle_buf_num][N]; // 用来存储多个数独题目的缓冲区

double now()
{ // 返回时间
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (double)tv.tv_sec * 1000000 + (double)tv.tv_usec;
}

// 初始化线程池
void init()
{
  if (DEBUG)
    cout << "init ThreadPool...." << '\n';
  ios_base::sync_with_stdio(SYNC);
  thread_num = thread::hardware_concurrency(); // 获取系统支持的线程数量
  thread_pool.start(thread_num);
  if (DEBUG)
    cout << "ThreadNum: " << thread_num << '\n';
}

// 销毁线程池
void del()
{
  if (DEBUG)
    cout << "destroy ThreadPool...." << '\n';
  thread_pool.close(); // 调用了线程池的关闭函数
  if (DEBUG)
    cout << "destroy success" << '\n';
}

// 解数独
int solve(int start, int end) // 接收两个参数，表示要处理的数独题目范围
{
  for (int i = start; i < end; i++)
    solve_sudoku_dancing_links(puzzle_buf[i]);
  if (DEBUG) // 如果 DEBUG 宏定义为真，会输出当前线程的 ID 和处理的数独题目范围
  {
    thread::id tid = this_thread::get_id();
    cout << "[tid]: " << tid << " | "
         << "[start]: " << start << " | "
         << "[end]: " << end << '\n';
  }
  return 0;
}

// 打印数独题目
void print(int line_num) // 接收一个参数，表示要打印的数独题目数量
{
  for (int i = 0; i < line_num; i++)
  {
    for (int j = 0; j < N; j++)
      putchar('0' + puzzle_buf[i][j]);
    putchar('\n');
  }
}

// 文件处理函数
void file_process(string file_name) // 参数是要处理的文件名
{
  // 如果DEBUG宏为真，则记录开始处理文件的时间
  if (DEBUG)
    total_time_start = now();

  // 打开一个输入文件流‘input_file’，用于读取文件内容。文件名由参数‘file_name’指定，
  // 打开模式为‘ios::in’,表示以文本输入模式打开文件
  ifstream input_file(file_name, ios::in);

  // 创建一个存储future<int>对象的向量‘results’，这里的‘future’可以理解为未来的任务或结果
  // future<int> 表示未来会得到一个整数类型的结果
  vector<future<int>> results;

  // 进入循环直到文件末尾
  while (!input_file.eof())
  {
    // 如果'DEBUG'为真，则记录当前处理一行数据的开始时间
    if (DEBUG)
      time_start = now();

    // 取数据

    int line_num = 0; // 初始化计数器‘line_num’用于记录当前处理的行数

    // 进入内部循环，直到文件末尾或到达最大处理行数限制
    while (!input_file.eof() && line_num < puzzle_buf_num)
    {
      // 使用‘getline’函数从文件中读取一行内容，存储到字符数组‘one_puzzle’中。
      // ‘N+1’表示每行最多读取 N 个字符，加上一个空字符作为字符串的结束符
      input_file.getline(one_puzzle, N + 1);

      // 如果读取的行长度>=N，则进行处理
      if (strlen(one_puzzle) >= N)
      {
        // 将读取的一行数据转换为数独题目的数字，并存储到‘puzzle_buf’中对应的位置
        for (int i = 0; i < N; i++)
          puzzle_buf[line_num][i] = one_puzzle[i] - 48;
        // 增加行数计数器，表示已经处理了一行数据
        line_num++;
      }
    };

    // 如果DEBUG为1，打印当前文件处理的总行数（上个while已统计）
    if (DEBUG)
      cout << "current data line_num: " << line_num << '\n';

    // 分数据

    // 初始化变量，用于分配任务给多个线程处理
    int start = 0, end = 0, len = (line_num + thread_num) / thread_num;

    // 循环将任务分配给多个线程处理，每个线程处理一部分数据
    for (int i = 0; i < thread_num; i++)
    {
      end = (start + len >= line_num) ? line_num : start + len;
      // 将每个线程处理的结果（使用‘future’包装）添加到‘result’向量中
      results.emplace_back(thread_pool.enqueue(solve, start, end));
      start += len;
    }
    if (DEBUG)
      cout << "wait task accomplish..." << '\n';

    // 等待所有线程的任务完成，即等待所有‘future’对象的结果就绪
    for (auto &&result : results)
      result.wait();

    // 输出数据
    if (DEBUG)
      cout << "print data" << '\n';
    print(line_num); // 打印处理过的数独题目数目

    // 输出时间统计信息
    if (DEBUG)
    {
      time_end = now();
      sec = (time_end - time_start) / 1000000.0;
      cout << "[time]:" << 1000 * sec << "ms" << '\n';
      cout << "------------------------------------------------------" << '\n';
    }
  }
  input_file.close(); // 关闭文件流，释放资源
  if (DEBUG)
  {
    total_time_end = now();
    sec = (total_time_end - total_time_start) / 1000000.0;
    cout << "[total time]:" << 1000 * sec << "ms" << '\n';
    cout << "------------------------------------------------------" << '\n';
  }
}
int main()
{
  init();
  string file_name;
  while (getline(cin, file_name))
    file_process(file_name);
  del();
  return 0;
}
