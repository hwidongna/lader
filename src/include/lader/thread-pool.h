#ifndef THREAD_POOL_H__
#define THREAD_POOL_H__

// A thread manager that allows submission of tasks to be run in different
// threads. (This was highly influenced by Moses's ThreadPool, but
// re-implemented and tweaked a bit.

#include <queue>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <pthread.h>

namespace lader {

class Task {
public:
    virtual void Run() = 0;
    virtual ~Task() { }
};

class ThreadPool {

public:
    // Create a thread pool with a certain number of threads
    ThreadPool(int size, int queue_limit = 0);

    // Submit a task for execution
    void Submit(Task * task);

    // Stop the remaining values
    void Stop(bool process_remaining);

protected:
    // The function executed by each thread, which will wait for a task, then
    // execute it when it is ready
    void Run();

    std::queue<Task*> tasks_;
    boost::thread_group threads_;
    boost::mutex mutex_;
    boost::condition_variable thread_needed_;
    boost::condition_variable thread_available_;
    bool stopped_;
    bool stopping_;
    int queue_limit_;

};

}

#endif
