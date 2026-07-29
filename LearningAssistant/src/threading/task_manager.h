/*
 * task_manager.h —— 后台任务管理器
 *
 * 知识点：
 *   1. std::future —— 获取异步任务结果
 *   2. std::async —— 便捷异步调用
 *   3. std::chrono —— 超时等待
 *   4. 使用线程池执行后台任务
 */
#pragma once

#include "threading/thread_pool.h"
#include <future>
#include <string>
#include <map>
#include <chrono>

namespace la {

/// @brief 后台任务管理器
/// 管理多个异步任务，支持任务提交、查询状态、等待完成
class TaskManager {
public:
    explicit TaskManager(ThreadPool& pool);

    /// 提交一个后台任务，返回任务ID
    template<typename Func>
    int submit(Func&& func);

    /// 等待指定任务完成
    template<typename Rep, typename Period>
    bool waitFor(int taskId, std::chrono::duration<Rep, Period> timeout);

    /// 查询任务状态
    bool isReady(int taskId);

private:
    ThreadPool& m_pool;
    int m_nextId = 1;
    std::map<int, std::future<void>> m_futures;  // STL: map 管理任务状态
    std::mutex m_mutex;
};

template<typename Func>
int TaskManager::submit(Func&& func) {
    std::lock_guard<std::mutex> lock(m_mutex);
    int id = m_nextId++;
    // 通过线程池提交任务
    m_futures[id] = m_pool.enqueue(std::forward<Func>(func));
    return id;
}

template<typename Rep, typename Period>
bool TaskManager::waitFor(int taskId, std::chrono::duration<Rep, Period> timeout) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_futures.find(taskId);
    if (it == m_futures.end()) return false;
    // future::wait_for: 等待指定时间，返回 future_status
    auto status = it->second.wait_for(timeout);
    return status == std::future_status::ready;
}

} // namespace la
