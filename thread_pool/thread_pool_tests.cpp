#include <random>
#include <chrono>
#include <thread>
#include <iostream>
#include <functional>

#include "thread_pool.hpp"

namespace test {
namespace thread_pool {

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

// // TEST THREADPOOL ON WORKING WITH DIFFERENT TASKS

// // Submit (partial) multiplication table
// for (int i = 1; i < 3; ++i) {
//     for (int j = 1; j < 10; ++j) {
//     tp.enqueue(multiply, i, j);
//     }
// }

// // Submit function with output parameter passed by ref
// int output_ref;
// auto future1 = tp.enqueue(multiply_output, std::ref(output_ref), 5, 6);

// // Wait for multiplication output to finish
// future1.get();
// std::cout << "Last operation result is equals to " << output_ref << std::endl;

// // Submit function with return parameter 
// auto future2 = tp.enqueue(multiply_return, 5, 3);

// // Wait for multiplication output to finish
// int res = future2.get();
// std::cout << "Last operation result is equals to " << res << std::endl;

// // TEST THREADPOOL ON WORKING CORRECTLY
// auto begin = std::chrono::high_resolution_clock::now();

// auto t1 = tp.enqueue([&](){
//     std::this_thread::sleep_for(std::chrono::milliseconds(10000));
//     std::cout << "Hi: 1" << std::endl;
//     return 1;
// });

// auto t2 = tp.enqueue([&](){
//     std::this_thread::sleep_for(std::chrono::milliseconds(5000));
//     std::cout << "Hi: 2" << std::endl;
//     return 2;
// });

// auto t3 = tp.enqueue([&](){
//     std::this_thread::sleep_for(std::chrono::milliseconds(7500));
//     std::cout << "Hi: 3" << std::endl;
//     return 3;
// });

// std::cout << t1.get() + t2.get() + t3.get() << std::endl;

// auto end = std::chrono::high_resolution_clock::now();
// auto time = std::chrono::duration_cast<std::chrono::milliseconds>(end-begin).count();
// std::cout << std::boolalpha << (time == 10000) << std::endl;
// std::cout << time << "ms" << std::endl;

} // thread_pool
} // test