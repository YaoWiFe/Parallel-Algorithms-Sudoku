#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <vector>

class ThreadPool
{
public:
  ThreadPool();    // 无参构造函数，用于创建线程池对象
  void start(int); // 声明一个参数为整数的成员函数，用于启动线程池并指定线程数量
  void close();    // 声明一个关闭线程池的成员函数
  // 声明一个成员模板函数 enqueue 用于向线程池添加任务并返回对应的‘std::future’对象
  template <class F, class... Args>
  auto enqueue(F &&f, Args &&...args)
      -> std::future<typename std::result_of<F(Args...)>::type>;
  ~ThreadPool(); // 声明了析构函数，用于销毁线程池对象

  // 下面的成员变量和成员函数被声明为私有，只能在类内部访问
private:
  std::vector<std::thread> workers;        // 用于存储线程对象的容器
  std::queue<std::function<void()>> tasks; // 用于存储任务的队列，任务是‘void’类型的函数
  std::mutex queue_mutex;                  // 用于保护任务队列的互斥量
  std::condition_variable condition;       // 条件变量，用于线程的同步
  bool stop;                               // 标志，表示线程池是否停止
};

// 定义了构造函数‘ThreadPool’，在其中对‘stop’进行了初始化
inline ThreadPool::ThreadPool() : stop(false)
{
}

// 开始定义start函数，它是ThreadPool类的成员函数，接受一个int类型的参数'threads'
// ‘start’函数的作用是初始化一个线程池，每个线程都处于等待任务的状态。
// 当有任务加入到任务队列时，线程会被唤醒并执行任务，直到线程池被显式停止。
inline void ThreadPool::start(int threads)
{

  // 循环‘threads’次，启动指定数量的线程
  for (size_t i = 0; i < threads; ++i)

    // 使用emplace_back函数再‘workers’容器的末尾创建一个新的线程对象，
    // 并将一个lambda表达式作为参数传递给‘emplace_back’函数，该表达式定义了每个线程的执行逻辑
    workers.emplace_back([this]
                         {
      for (;;) { //无限循环，每个线程都会持续地从任务队列中取出任务并执行，
                 //直到线程池被关闭或者任务队列为空。
          
        std::function<void()> task; //用于存储要执行的任务
        //以下{}代码块内是每个线程的主要执行逻辑 
        {
          /* 创建一个 std::unique_lock 对象 lock，
          并使用 std::mutex 对象 queue_mutex 进行加锁。
          这样做是为了保护任务队列的访问，防止多个线程同时访问任务队列导致数据竞争 */ 
          std::unique_lock<std::mutex> lock(this->queue_mutex);

          /* 调用 condition 条件变量的 wait 函数，将当前线程挂起，
          直到 stop 标志为真（即线程池被关闭）或者任务队列不为空。
          在等待期间，会释放 lock 对象的锁，以允许其他线程访问任务队列*/
          this->condition.wait(
              lock, [this] { return this->stop || !this->tasks.empty(); });

          //如果线程池被关闭且任务队列为空，则直接返回，结束线程的执行
          if (this->stop && this->tasks.empty())
            return;

          //将队列首部的任务移动到task变量中，并从队列中移除该任务。
          //这里使用了std::move以避免复制，提高效率。
          task = std::move(this->tasks.front());
          this->tasks.pop();
        }

        //执行取出的任务，即调用存储在‘task’对象中的函数  
        task();
      } });
}

// 关闭线程池函数
inline void ThreadPool::close()
{
  stop = true;
}

// 定义了ThreadPool类的模板成员函数enqueue，用于向线程池添加新的工作任务。
// 这个函数接受一个可调用对象（如函数、函数对象、lambda表达式等）F和任意数量的参数Args，
// 并将它们封装为一个任务，异步地加入到任务队列中执行
template <class F, class... Args>
auto ThreadPool::enqueue(F &&f, Args &&...args)
    -> std::future<typename std::result_of<F(Args...)>::type>
{
  using return_type = typename std::result_of<F(Args...)>::type;

  auto task = std::make_shared<std::packaged_task<return_type()>>(
      std::bind(std::forward<F>(f), std::forward<Args>(args)...));

  std::future<return_type> res = task->get_future();
  {
    std::unique_lock<std::mutex> lock(queue_mutex);

    if (stop)
      throw std::runtime_error("enqueue on stopped ThreadPool");

    tasks.emplace([task]()
                  { (*task)(); });
  }
  condition.notify_one();
  return res;
}

// 定义了析构函数，其主要职责是在类实例被销毁前，确保所有工作线程正确地完成它们的任务并终止。
// 通过设置停止标志、通知所有工作线程，并等待它们全部完成，确保了线程池能够安全且干净地关闭，防止了资源泄露和未定义行为的发生
inline ThreadPool::~ThreadPool()
{
  {
    std::unique_lock<std::mutex> lock(queue_mutex);
    stop = true;
    condition.notify_all();
  }
  for (std::thread &worker : workers)
    worker.join();
}

#endif