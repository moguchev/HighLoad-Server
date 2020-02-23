#include "thread_pool.hpp"

void thread_pool::start(std::size_t n = 1) {
    for (std::size_t i = 0; i < n; ++i)
    {
        // each thread is a std::async running this->thread_task():
        finished.push_back(
                std::async(
                        std::launch::async,
                        [this]{ thread_task(); }
                )
        );
    }
}

void thread_pool::abort() {
    cancel_pending();
    finish();
}

void thread_pool::cancel_pending() {
    std::unique_lock<std::mutex> ul(mtx);
    work.clear();
}

void thread_pool::finish() {
    {
        std::unique_lock<std::mutex> ul(mtx);
        for(auto&&unused:finished){
            work.push_back({});
        }
    }
    cv.notify_all();
    finished.clear();
}

thread_pool::~thread_pool() {
    finish();
}

void thread_pool::thread_task() {
    while(true){
        // pop a task off the queue:
        std::packaged_task<void()> f;
        {
            // usual thread-safe queue code:
            std::unique_lock<std::mutex> ul(mtx);
            if (work.empty()){
                cv.wait(ul,[&]{return !work.empty();});
            }
            f = std::move(work.front());
            work.pop_front();
        }
        // if the task is invalid, it means we are asked to abort:
        if (!f.valid()) return;
        // otherwise, run the task:
        f();
    }
}