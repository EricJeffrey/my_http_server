
#if !defined(THREAD_POOL_H)
#define THREAD_POOL_H

#include "logger.h"
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

using std::condition_variable;
using std::mutex;
using std::queue;
using std::thread;
using std::unique_lock;
using std::vector;

// 线程池任务参数
struct task_arg {
    int fd;
    task_arg() : fd(0) {}
    task_arg(int f) : fd(f) {}
};

typedef void (*runnable)(task_arg arg);
// 线程池中待执行任务
struct task {
    runnable runner;
    task_arg arg;

    task(runnable r, task_arg a) {
        runner = r;
        arg = a;
    }
    void run() {
        LOGGER_FORMAT(LOG_LV_VERBOSE, "task: run now, arg fd: %d", arg.fd);
        runner(arg);
    }
};

// 线程池
class thread_pool {
private:
    const int thread_num;
    vector<thread> threads;
    condition_variable condq;

    queue<task> qtasks;
    mutex mutexq;

    thread_pool(int thr_num) : thread_num(thr_num) {
        for (int i = 0; i < thread_num; i++) {
            threads.push_back(thread([this]() {
                while (1) {
                    unique_lock<mutex> lck(mutexq);
                    while (qtasks.empty())
                        condq.wait(lck);
                    task tmptask = qtasks.front();
                    lck.unlock();
                    lck.release();
                    LOGGER_SIMP(LOG_LV_DEBUG, "thread: task got, lck unlocked, running...");
                    tmptask.run();
                }
            }));
        }
    }

public:
    // 提交任务，通知可用线程执行
    void pushTask(task t) {
        LOGGER_SIMP(LOG_LV_VERBOSE, "thread pool: push task");
        mutexq.lock();
        qtasks.push(t);
        mutexq.unlock();
        condq.notify_one();
    }

    thread_pool(thread_pool &) = delete;
    void operator=(thread_pool const &) = delete;
    // 获取线程池实例
    static thread_pool &getInstance(int tot_thread_num) {
        static thread_pool pool(tot_thread_num);
        return pool;
    }
};

#endif // THREAD_POOL_H
