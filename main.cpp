#include <chrono>
#include <condition_variable>
#include <future>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>


class ThreadPool {
public:
    ThreadPool() = default;

    ThreadPool(const ThreadPool&) = delete;

    ThreadPool &operator=(const ThreadPool&) = delete;

        // start N threads in the thread pool.
    void start(std::size_t n = 1){
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

    // queue( lambda ) will enqueue the lambda into the tasks for the threads
    // to use.  A future of the type the lambda returns is given to let you get
    // the result out.
    template<class T, class R=std::result_of_t<T&()>>
    auto enqueue(T&& task)->std::future<R>{
        // wrap the function object into a packaged task, splitting
        // execution from the return value:
        auto wrapper = std::make_shared<std::packaged_task<R()>>(std::forward<T>(task));

        auto r = wrapper->get_future(); // get the return value before we hand off the task
        {
            std::unique_lock<std::mutex> l(EventMtx);
            work.emplace_back(std::move(*wrapper)); // store the task<R()> as a task<void()>
        }
        v.notify_one(); // wake a thread to work on the task

        return r; // return the future result of the task
    }

    // abort() cancels all non-started tasks, and tells every working thread
    // stop running, and waits for them to finish up.
    void abort() {
        cancel_pending();
        finish();
    }

    // cancel_pending() merely cancels all non-started tasks:
    void cancel_pending() {
        std::unique_lock<std::mutex> l(EventMtx);
        work.clear();
    }

    // finish enques a "stop the thread" message for every thread, then waits for them:
    void finish() {
        {
            std::unique_lock<std::mutex> l(EventMtx);
            for(auto&&unused:finished){
                work.push_back({});
            }
        }
        v.notify_all();
        finished.clear();
    }

    ~ThreadPool() {
        finish();
    }
private:
    // the mutex, condition variable and deque form a single
    // thread-safe triggered queue of tasks:
    std::mutex EventMtx;
    std::condition_variable v;
    // note that a packaged_task<void> can store a packaged_task<R>:
    std::deque<std::packaged_task<void()>> work;

    // this holds futures representing the worker threads being done:
    std::vector<std::future<void>> finished;

    // the work that a worker thread does:
    void thread_task() {
        while(true){
            // pop a task off the queue:
            std::packaged_task<void()> f;
            {
                // usual thread-safe queue code:
                std::unique_lock<std::mutex> l(EventMtx);
                if (work.empty()){
                    v.wait(l,[&]{return !work.empty();});
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
};

int main()
{
    ThreadPool tp;

    tp.start(3);

    auto begin = std::chrono::high_resolution_clock::now();

    auto t1 = tp.enqueue([&](){
        std::this_thread::sleep_for(std::chrono::milliseconds(10000));
        std::cout << "Hi: 1" << std::endl;
        return 1;
    });

    auto t2 = tp.enqueue([&](){
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
        std::cout << "Hi: 2" << std::endl;
        return 2;
    });

    auto t3 = tp.enqueue([&](){
        std::this_thread::sleep_for(std::chrono::milliseconds(7500));
        std::cout << "Hi: 3" << std::endl;
        return 3;
    });

    std::cout << t1.get() + t2.get() + t3.get() << std::endl;
    
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(end-begin).count()<< "ms"<< std::endl;
    
}
