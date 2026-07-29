/*
 * thread_pool.h —— 简单线程池
 *
 * 知识点：
 *   1. std::thread           —— 线程创建与管理
 *   2. std::mutex + std::condition_variable —— 生产者-消费者模式
 *   3. std::queue + std::function —— 任务队列
 *   4. std::future + std::packaged_task —— 获取异步结果
 *   5. std::atomic           —— 无锁原子操作
 *   6. RAII 生命周期管理      —— 析构时自动停止所有线程
 */
#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>
#include <type_traits>

namespace la {

/// @brief 固定大小线程池
/// 使用生产者-消费者模式：主线程生产任务，工作线程消费任务
class ThreadPool {
public:
    /// @param numThreads 线程数，0 表示 hardware_concurrency
    explicit ThreadPool(size_t numThreads = 0);
    ~ThreadPool();

    // 禁止拷贝
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    /// @brief 提交任务并获取 future
    /// 用法: auto future = pool.enqueue([](int x) { return x * 2; }, 42);
    template<typename Func, typename... Args>
    auto enqueue(Func&& func, Args&&... args)
        -> std::future<typename std::result_of<Func(Args...)>::type>;

    /// @brief 获取活跃线程数
    size_t activeCount() const { return m_activeCount.load(); }

    /// @brief 获取队列中等待的任务数
    size_t pendingCount() const;

private:
    void workerLoop();  // 每个工作线程的主循环

    std::vector<std::thread> m_workers;              // 工作线程列表
    std::queue<std::function<void()>> m_tasks;        // 任务队列
    std::mutex m_queueMutex;                          // 保护任务队列
    std::condition_variable m_condition;               // 通知工作线程
    std::atomic<bool> m_stop{false};                  // 停止标志
    std::atomic<size_t> m_activeCount{0};             // 活跃线程计数
};

// ========== 模板实现 ==========

template<typename Func, typename... Args>
auto ThreadPool::enqueue(Func&& func, Args&&... args)
    -> std::future<typename std::result_of<Func(Args...)>::type>
{
    using ReturnType = typename std::result_of<Func(Args...)>::type;

    // std::packaged_task: 将可调用对象包装为异步任务，通过 future 获取结果
    // std::shared_ptr: 因为 packaged_task 不可拷贝，用 shared_ptr 包装放入 lambda
    auto task = std::make_shared<std::packaged_task<ReturnType()>>(
        std::bind(std::forward<Func>(func), std::forward<Args>(args)...)
    );

    std::future<ReturnType> result = task->get_future();

    {
        // std::unique_lock: 比 lock_guard 更灵活，支持手动解锁和条件变量
        std::unique_lock<std::mutex> lock(m_queueMutex);

        if (m_stop) {
            throw std::runtime_error("Cannot enqueue on stopped ThreadPool");
        }

        // 将 packaged_task 包装为 void() 函数放入队列
        m_tasks.emplace([task]() { (*task)(); });
    }

    // 通知一个等待的工作线程
    m_condition.notify_one();
    return result;
}

} // namespace la
