// Listening 2.8.

#include <algorithm>
#include <chrono>
#include <functional>
#include <iostream>
#include <numeric>
#include <thread>
#include <vector>

template <typename Iterator, typename T>
struct accumulate_block
{
    void operator()(Iterator first, Iterator last, T &result)
    {
        result = std::accumulate(first, last, result);
    }
};

template <typename Iterator, typename T>
T parallel_accumulate(Iterator first, Iterator last, T init)
{
    unsigned long const length = std::distance(first, last);

    if (!length)
        return init;

    unsigned long const min_per_thread = 25;
    unsigned long const max_threads = (length + min_per_thread - 1) / min_per_thread;

    unsigned long const hardware_threads = std::thread::hardware_concurrency();
    unsigned long const num_threads = std::min(hardware_threads != 0 ? hardware_threads : 2, max_threads);

    unsigned long const block_size = length / num_threads;

    std::vector<T> results(num_threads);
    std::vector<std::thread> threads(num_threads - 1);

    Iterator block_start = first;
    for (unsigned long i = 0; i < (num_threads - 1); ++i)
    {
        Iterator block_end = block_start;
        std::advance(block_end, block_size);
        threads[i] = std::thread(accumulate_block<Iterator, T>(), block_start, block_end, std::ref(results[i]));
        block_start = block_end;
    }
    accumulate_block<Iterator, T>()(block_start, last, results[num_threads - 1]);

    std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));

    return std::accumulate(results.begin(), results.end(), init);
}

int main()
{
    std::vector<int> numbers(100000000, 1);

    auto start = std::chrono::high_resolution_clock::now();
    int single_threaded_sum = std::accumulate(numbers.begin(), numbers.end(), 0);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> single_threaded_duration = end - start;
    std::cout << "Single-threaded sum: " << single_threaded_sum << "\n";
    std::cout << "Single-threaded duration: " << single_threaded_duration.count() << " seconds\n";

    start = std::chrono::high_resolution_clock::now();
    int multi_threaded_sum = parallel_accumulate(numbers.begin(), numbers.end(), 0);
    end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> multi_threaded_duration = end - start;
    std::cout << "Multi-threaded sum: " << multi_threaded_sum << "\n";
    std::cout << "Multi-threaded duration: " << multi_threaded_duration.count() << " seconds\n";

    std::cout << std::thread::hardware_concurrency() << "\n";

    return 0;
}