#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <condition_variable>
#include <thread>
#include <future>
#include <vector>
#include <queue>
#include <memory>
#include <functional>
#include <mutex>

class thread_pool {
public:
    thread_pool() = default;

    // queue( lambda ) will enqueue the lambda into the tasks for the threads
    // to use. A future of the type the lambda returns is given to let you get
    // the result out.
    template<class T, class...Args>
    auto enqueue(T&& task, Args&&... args)->std::future<decltype(task(args...))>;

    // start n threads in the thread pool.
    void start(std::size_t n);

    // abort() cancels all non-started tasks, and tells every working thread
    // stop running, and waits for them to finish up.
    void abort();

    // cancel_pending() merely cancels all non-started tasks:
    void cancel_pending();

    // finish enques a "stop the thread" message for every thread, then waits for them:
    void finish();

    ~thread_pool();
private:
    thread_pool(const thread_pool &) = delete;
    thread_pool(thread_pool &&) = delete;

    thread_pool & operator=(const thread_pool &) = delete;
    thread_pool & operator=(thread_pool &&) = delete;
    // the mutex, condition variable and deque form a single
    // thread-safe triggered queue of tasks:
    std::mutex mtx;
    std::condition_variable cv;
    // note that a packaged_task<void> can store a packaged_task<R>:
    std::deque<std::packaged_task<void()>> work;

    // this holds futures representing the worker threads being done:
    std::vector<std::future<void>> finished;

    // the work that a worker thread does:
    void thread_task();
};

template<class T, class...Args>
auto thread_pool::enqueue(T&& task, Args&&... args)->std::future<decltype(task(args...))> {
    // wrap the function object into a packaged task, splitting
    // execution from the return value:
    std::function<decltype(task(args...))()> func = std::bind(std::forward<T>(task), std::forward<Args>(args)...);
    auto wrapper = std::make_shared<std::packaged_task<decltype(task(args...))()>>(func);

    auto r = wrapper->get_future(); // get the return value before we hand off the task
    {
        std::unique_lock<std::mutex> ul(mtx);
        work.emplace_back(std::move(*wrapper)); // store the task<R()> as a task<void()>
    }
    cv.notify_one(); // wake a thread to work on the task

    return r; // return the future result of the task
}

#endif // THREAD_POOL_HPP