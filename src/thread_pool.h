
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

typedef void (*runnable)(void *arg);

// 线程池中待执行任务
struct task {
    runnable runner;
    void *arg;

    task(runnable r, void *a) {
        runner = r;
        arg = a;
    }
    void run() {
        runner(arg);
        free(arg);
    }
};

// 线程池
class thread_pool {
private:
    const int thread_num;
    vector<thread> threads;
    condition_variable condq;
    mutex mutex_ava;

    queue<task> qtasks;
    mutex mutexq;

    thread_pool(int thr_num) : thread_num(thr_num) {
        for (int i = 0; i < thread_num; i++) {
            threads.push_back(thread([this]() {
                while (1) {
                    task tmptask = this->frontTask();
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
    // 获取任务
    task frontTask() {
        LOGGER_SIMP(LOG_LV_VERBOSE, "thread pool: front task");
        unique_lock<mutex> lck(mutexq);
        while (qtasks.empty())
            condq.wait(lck);
        task tmptask = qtasks.front();
        qtasks.pop();
        return tmptask;
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
