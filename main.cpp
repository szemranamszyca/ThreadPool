#include <iostream>
#include <thread>
#include <mutex>
#include <string>
#include <queue>
#include <vector>
#include <functional>
#include <condition_variable>
#include <algorithm>

static unsigned int hardware_threads = std::thread::hardware_concurrency();
std::mutex ioMutex_;

class ThreadPool
{
    public:
        ThreadPool(std::size_t numOfWorkers) : 
            numOfWorkers_(numOfWorkers),
            workers_(numOfWorkers_)
        {
            for(std::size_t i = 0; i < numOfWorkers_; ++i)
            {
               workers_.push_back(std::thread(&ThreadPool::loop, this, 2));
            }
        }

        ~ThreadPool()
        {
            std::for_each(workers_.begin(), workers_.end(), std::mem_fn(&std::thread::join));
        }

        void pushVal(int val)
        {
            std::unique_lock<std::mutex> qlock(queueMutex_);
            queue_.push(val);
            queue_empty_.notify_one();
        }

    private:
        void loop(uint16_t worktime)
        {
            while(true)
            {
                int val;

                {
                    std::unique_lock<std::mutex> qlock(queueMutex_);
                    queue_empty_.wait(qlock, [this]
                    {
                        std::cout << "Notified!" << std::endl;
                        return !queue_.empty();
                    });
                    val = queue_.front();
                    queue_.pop();
                }

                {
                    std::unique_lock<std::mutex> iolock(ioMutex_);
                    std::cout << "Working with value " << val << " for " << worktime << " seconds in thread: " << std::this_thread::get_id() << std::endl;
                }
                
                std::this_thread::sleep_for(std::chrono::seconds(worktime));
                
                {
                    std::unique_lock<std::mutex> iolock(ioMutex_);
                    std::cout << "Thread " << std::this_thread::get_id() << " is done" << std::endl;
                }
            }
        }

        std::size_t numOfWorkers_;
        std::vector<std::thread> workers_;
        
        std::mutex queueMutex_;

        std::queue<int> queue_;
        std::condition_variable queue_empty_;
};

int main()
{
    {
        std::unique_lock<std::mutex> iolock(ioMutex_);
        std::cout << "Hello from main! Threads: " << hardware_threads << '\n';
    }
    ThreadPool tp(hardware_threads-1);
    while(true)
    {
        int val;
        std::cin >> val;
        tp.pushVal(val);
    }
    return 0;
}
