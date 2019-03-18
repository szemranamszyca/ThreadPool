#include <iostream>
#include <thread>
#include <mutex>
#include <cstdlib>
#include <string>
#include <queue>
#include <vector>
#include <functional>

static unsigned int hardware_threads = std::thread::hardware_concurrency();


std::mutex global_mutex;

struct Job
{
    Job(const std::string& name) : name_(name){};
    std::string name_;
    void process()
    {
        std::cout << name_ << " " << std::this_thread::get_id() << " start!\n";
        std::this_thread::sleep_for(std::chrono::seconds(4));
        std::cout << name_ << " " << std::this_thread::get_id() << " end!\n";
    }
};

void dummy()
{
    {
        std::lock_guard<std::mutex> gl(global_mutex);
        std::cout << "Hej " << std::this_thread::get_id() << '\n';
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
}


class ThreadPool
{
    public:
        ThreadPool() : workers_(workersNum_)
        {
            for(std::size_t i = 0; i < workersNum_; i++)
            {
                workers_.push_back(std::thread(&ThreadPool::loop_func, this));
                // workers_.push_back(std::thread(dummy));
            }
        }
        ~ThreadPool()
        {
            // for(std::size_t i = 0; i < workersNum_; i++)
            // {
            //     workers_[i].join();
            // }
            std::for_each(workers_.begin(), workers_.end(), std::mem_fn(&std::thread::join));
        }
        void add_job(std::function<void()> job)
        {
            {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                queue_jobs_.push(job);
            }
            queue_cond_.notify_one();
        }
    private:
        void loop_func()
        {
            while(true)
            {
                std::function<void()> job;
                {
                    {
                        std::unique_lock<std::mutex> iolock(global_mutex);
                        std::cout << "Thread: " << std::this_thread::get_id() <<
                        " waiting for non-empty queue: \n";
                    }

                    std::unique_lock<std::mutex> lock(queue_mutex_);
                    queue_cond_.wait(lock, [this]{return !queue_jobs_.empty();});

                    {
                        std::unique_lock<std::mutex> iolock(global_mutex);
                        std::cout << "Job on queue " << std::this_thread::get_id() << " took it " << '\n';
                    }

                    job = queue_jobs_.front();
                    queue_jobs_.pop();
                    
                }
                job();
                
            }
        }
        std::size_t workersNum_ = 6;
        std::vector<std::thread> workers_;
        std::mutex queue_mutex_;
        std::condition_variable queue_cond_;
        std::queue<std::function<void()>> queue_jobs_;
};

int main()
{
    ThreadPool tp;

    Job job1("first");
    Job job2("second");

    tp.add_job(std::bind(&Job::process, &job1));
    tp.add_job(std::bind(&Job::process, &job2));

    while(true)
    {}

    
    return 0;
}