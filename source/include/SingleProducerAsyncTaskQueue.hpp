#ifndef SINGLE_PRODUCER_ASYNC_TASK_QUEUE_HPP
#define SINGLE_PRODUCER_ASYNC_TASK_QUEUE_HPP

#include <cstdint>
#include <thread>
#include <mutex>
#include <future>
#include <functional>
#include <queue>

//******************************************************************************
// SingleProducerAsyncTaskQueue
//******************************************************************************

/**
 * A single-producer asynchronous task queue.
 *
 * Tasks are submitted only from the owning thread.
 * Worker threads execute tasks but must not submit new tasks.
 *
 * Destruction waits until all queued tasks have completed.
 */

class SingleProducerAsyncTaskQueue
{

public:

    SingleProducerAsyncTaskQueue();
    ~SingleProducerAsyncTaskQueue();

    template<typename F, typename... Args>
    auto enqueue_task(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>>;

    auto queue_size() -> int64_t;

private:

    std::thread                       m_thread;
    std::mutex                        m_mutex;
    std::condition_variable           m_condition_variable;
    std::queue<std::function<void()>> m_task_queue;
    bool                              m_stop = false;

    auto thread_loop() -> void;

};

template<typename F, typename... Args>
auto SingleProducerAsyncTaskQueue::enqueue_task(F&& f, Args&&... args)
    -> std::future<std::invoke_result_t<F, Args...>>
{
    using ReturnType = std::invoke_result_t<F, Args...>;

    auto task = std::make_shared<std::packaged_task<ReturnType()>> (
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );

    auto result = task->get_future();

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_task_queue.push([task]() { (*task)(); });
    }

    m_condition_variable.notify_one();
    return result;
}

#ifdef ARRAY_IMPLEMENTATION

SingleProducerAsyncTaskQueue::SingleProducerAsyncTaskQueue()
{
    m_thread = std::thread([this] { thread_loop(); });
}

SingleProducerAsyncTaskQueue::~SingleProducerAsyncTaskQueue()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stop = true;
    }
    m_condition_variable.notify_one();
    if (m_thread.joinable())
    {
        m_thread.join();
    }
}

auto SingleProducerAsyncTaskQueue::queue_size() -> int64_t
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_task_queue.size();
}

auto SingleProducerAsyncTaskQueue::thread_loop() -> void
{
    while (true)
    {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_condition_variable.wait(lock, [this] { return m_stop || !m_task_queue.empty(); });

            if (m_stop && m_task_queue.empty())
            {
                break;
            }

            task = std::move(m_task_queue.front());
            m_task_queue.pop();
        }
        task();
    }
}

#endif

//******************************************************************************
// SingleProducerAsyncTaskQueuePool
//******************************************************************************

class SingleProducerAsyncTaskQueuePool
{

public:

    explicit SingleProducerAsyncTaskQueuePool(int64_t task_queue_count = 1);

    template<typename F, typename... Args>
    auto enqueue_task(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>>;

    auto queue_size(int64_t queue_index) -> int64_t;

private:

    std::vector<SingleProducerAsyncTaskQueue> m_task_queues;

};

template<typename F, typename... Args>
auto SingleProducerAsyncTaskQueuePool::enqueue_task(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>>
{
    int64_t min_queue_size = std::numeric_limits<int64_t>::max();
    int64_t min_queue_size_queue_index = std::numeric_limits<int64_t>::max();

    for(int64_t i = 0; i < m_task_queues.size(); i++)
    {
        if(m_task_queues[i].queue_size() < min_queue_size)
        {
            min_queue_size = m_task_queues[i].queue_size();
            min_queue_size_queue_index = i;
        }
    }

    return m_task_queues[min_queue_size_queue_index].enqueue_task (
        std::forward<F>(f),
        std::forward<Args>(args)...
    );
}

#ifdef ARRAY_IMPLEMENTATION

SingleProducerAsyncTaskQueuePool::SingleProducerAsyncTaskQueuePool(int64_t task_queue_count) : m_task_queues(task_queue_count)
{
    if ( task_queue_count <= 0 )
    {
        throw_with_context<std::domain_error>("Domain error. Check source location.");
    }
}

auto SingleProducerAsyncTaskQueuePool::queue_size(int64_t queue_index) -> int64_t
{
    return m_task_queues[queue_index].queue_size();
}

#endif

#endif // SINGLE_PRODUCER_ASYNC_TASK_QUEUE_HPP