#include "ThreadPoolTester.h"
#include <iostream>
#include <cassert>
namespace utils {

void testSingleTaskExecution() {
    ThreadPool pool(4); // Create a pool with 4 threads

    // A future to hold the result of the asynchronous task
    auto futureResult = pool.PushTask([]() -> int {
        return 42;
        });

    // Get the result from the future and check if it's correct
    assert(futureResult.get() == 42);
    cout << "Test 1 (Single Task Execution) passed." << endl;


}

void testMultipleTasksExecution() {
    ThreadPool pool(4); // Assume we have 4 worker threads

    vector<future<int>> results;

    // Push multiple tasks
    for (int i = 0; i < 20; ++i) {
        results.push_back(pool.PushTask([i]() -> int {
            return i * i;
            }));
    }

    // Check all results
    for (int i = 0; i < 20; ++i) {
        assert(results[i].get() == i * i);
    }
    cout << "Test 2 (Multiple Tasks Execution) passed." << endl;
}

void testShutdownAndCleanup() {
    ThreadPool pool(2); // Smaller pool for quick shutdown

    // Push some tasks
    for (int i = 0; i < 10; ++i) {
        pool.PushTask([i]() {
            std::cout << "Task : " << i << std::endl;
            this_thread::sleep_for(chrono::milliseconds(1000)); // Simulate work
            return i;
            });
    }

    // Destruction of the pool should wait for all tasks to complete
}
}



int main()
{
    utils::testSingleTaskExecution();
    utils::testMultipleTasksExecution();
    utils::testShutdownAndCleanup();
}
