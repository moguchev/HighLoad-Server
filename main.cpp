#include <iostream>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <future>
#include <vector>
#include <queue>
#include <chrono>
#include <random>
#include <functional>

class ThreadPool {
public:
    ThreadPool() = default;

    ThreadPool(const ThreadPool &) = delete;
    ThreadPool(ThreadPool &&) = delete;

    ThreadPool & operator=(const ThreadPool &) = delete;
    ThreadPool & operator=(ThreadPool &&) = delete;

    // queue( lambda ) will enqueue the lambda into the tasks for the threads
    // to use.  A future of the type the lambda returns is given to let you get
    // the result out.
    template<class T, class...Args>
    auto enqueue(T&& task, Args&&... args)->std::future<decltype(task(args...))>{
        // wrap the function object into a packaged task, splitting
        // execution from the return value:
        std::function<decltype(task(args...))()> func = std::bind(std::forward<T>(task), std::forward<Args>(args)...);
        auto wrapper = std::make_shared<std::packaged_task<decltype(task(args...))()>>(func);

        auto r = wrapper->get_future(); // get the return value before we hand off the task
        {
            std::unique_lock<std::mutex> l(EventMtx);
            work.emplace_back(std::move(*wrapper)); // store the task<R()> as a task<void()>
        }
        v.notify_one(); // wake a thread to work on the task

        return r; // return the future result of the task
    }

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


std::random_device rd;
std::mt19937 mt(rd());
std::uniform_int_distribution<int> dist(-1000, 1000);
auto rnd = std::bind(dist, mt);


void simulate_hard_computation() {
  std::this_thread::sleep_for(std::chrono::milliseconds(2000 + rnd()));
}

// Simple function that adds multiplies two numbers and prints the result
void multiply(const int a, const int b) {
  simulate_hard_computation();
  const int res = a * b;
  std::cout << a << " * " << b << " = " << res << std::endl;
}

// Same as before but now we have an output parameter
void multiply_output(int & out, const int a, const int b) {
  simulate_hard_computation();
  out = a * b;
  std::cout << a << " * " << b << " = " << out << std::endl;
}

// Same as before but now we have an output parameter
int multiply_return(const int a, const int b) {
  simulate_hard_computation();
  const int res = a * b;
  std::cout << a << " * " << b << " = " << res << std::endl;
  return res;
}


int main()
{
    ThreadPool tp;
    tp.start(3);

    // TEST THREADPOOL ON WORKING WITH DIFFERENT TASKS

    // Submit (partial) multiplication table
    for (int i = 1; i < 3; ++i) {
        for (int j = 1; j < 10; ++j) {
        tp.enqueue(multiply, i, j);
        }
    }

    // Submit function with output parameter passed by ref
    int output_ref;
    auto future1 = tp.enqueue(multiply_output, std::ref(output_ref), 5, 6);

    // Wait for multiplication output to finish
    future1.get();
    std::cout << "Last operation result is equals to " << output_ref << std::endl;

    // Submit function with return parameter 
    auto future2 = tp.enqueue(multiply_return, 5, 3);

    // Wait for multiplication output to finish
    int res = future2.get();
    std::cout << "Last operation result is equals to " << res << std::endl;

    // TEST THREADPOOL ON WORKING CORRECTLY
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
    auto time = std::chrono::duration_cast<std::chrono::milliseconds>(end-begin).count();
    std::cout << std::boolalpha << (time == 10000) << std::endl;
    std::cout << time << "ms" << std::endl;
}
