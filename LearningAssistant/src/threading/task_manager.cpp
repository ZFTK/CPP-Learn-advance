/*
 * task_manager.cpp —— 后台任务管理器实现
 */
#include "threading/task_manager.h"

namespace la {

TaskManager::TaskManager(ThreadPool& pool) : m_pool(pool) {}

bool TaskManager::isReady(int taskId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_futures.find(taskId);
    if (it == m_futures.end()) return false;
    // future::wait_for 零超时：立即返回当前状态
    return it->second.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

} // namespace la
