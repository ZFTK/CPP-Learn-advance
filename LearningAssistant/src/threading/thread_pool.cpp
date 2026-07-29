/*
 * thread_pool.cpp —— 线程池实现
 */
#include "threading/thread_pool.h"
#include "core/logger.h"

namespace la {

ThreadPool::ThreadPool(size_t numThreads) {
    if (numThreads == 0) {
        // std::thread::hardware_concurrency: 获取硬件支持的并发线程数
        numThreads = std::thread::hardware_concurrency();
        if (numThreads == 0) numThreads = 4;  // 保底
    }

    LA_LOG_INFO("Creating thread pool with " + std::to_string(numThreads) + " threads");

    // 创建工作线程
    m_workers.reserve(numThreads);
    for (size_t i = 0; i < numThreads; ++i) {
        // std::thread 构造：传入成员函数 + this指针
        m_workers.emplace_back(&ThreadPool::workerLoop, this);
    }
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(m_queueMutex);
        m_stop = true;
    }
    // 唤醒所有等待的线程
    m_condition.notify_all();

    // 等待所有线程结束
    for (auto& worker : m_workers) {
        if (worker.joinable()) {
            worker.join();  // 阻塞直到线程结束
        }
    }
    LA_LOG_INFO("Thread pool destroyed");
}

void ThreadPool::workerLoop() {
    while (true) {
        std::function<void()> task;

        {
            // unique_lock: 配合 condition_variable 使用
            std::unique_lock<std::mutex> lock(m_queueMutex);

            // condition_variable::wait: 等待条件满足
            // lambda 返回 true 时继续，false 时释放锁并阻塞
            m_condition.wait(lock, [this] {
                return m_stop || !m_tasks.empty();
            });

            // 停止且队列为空 → 退出
            if (m_stop && m_tasks.empty()) return;

            // 取出一个任务
            task = std::move(m_tasks.front());
            m_tasks.pop();
        }

        // 执行任务（在锁外执行，允许其他线程并发取任务）
        m_activeCount++;
        task();
        m_activeCount--;
    }
}

size_t ThreadPool::pendingCount() const {
    // 注意：这个函数不是线程安全的，演示用
    // 实际项目应加锁或使用 atomic
    return 0;  // 简化实现，实际应加锁读取 m_tasks.size()
}

} // namespace la
