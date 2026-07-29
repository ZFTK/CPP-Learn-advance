/*
 * thread_demo.cpp —— 多线程速成示例（Windows API版，兼容老MinGW）
 *
 * 编译: g++ -std=c++14 thread_demo.cpp -o thread_demo
 *
 * 生产环境推荐 C++11 std::thread（需要 GCC 7+ / MSVC）
 * 完整线程池: LearningAssistant/src/threading/
 */
#define _WIN32_WINNT 0x0600
#include <iostream>
#include <windows.h>
#include <vector>

// ==================== 1. 基础线程 ====================
DWORD WINAPI threadFunc(LPVOID lpParam) {
    int id = *(int*)lpParam;
    std::cout << "  线程" << id << ": 工作中...\n";
    Sleep(1000);
    std::cout << "  线程" << id << ": 完成\n";
    return 0;
}

void demo_basic_thread() {
    std::cout << "=== 基础线程 (CreateThread) ===\n";
    int id1 = 1, id2 = 2;
    HANDLE t1 = CreateThread(nullptr, 0, threadFunc, &id1, 0, nullptr);
    HANDLE t2 = CreateThread(nullptr, 0, threadFunc, &id2, 0, nullptr);
    // WaitForSingleObject: 等待线程结束
    WaitForSingleObject(t1, INFINITE);
    WaitForSingleObject(t2, INFINITE);
    CloseHandle(t1); CloseHandle(t2);
    std::cout << "  C++11等价: std::thread t(func); t.join();\n---\n";
}

// ==================== 2. 互斥锁 ====================
CRITICAL_SECTION g_cs;
int g_counter = 0;

DWORD WINAPI incrementFunc(LPVOID lpParam) {
    int times = *(int*)lpParam;
    for (int i = 0; i < times; ++i) {
        // EnterCriticalSection: 获取锁
        EnterCriticalSection(&g_cs);
        ++g_counter;
        // LeaveCriticalSection: 释放锁
        LeaveCriticalSection(&g_cs);
    }
    return 0;
}

void demo_mutex() {
    std::cout << "=== 互斥锁 (CriticalSection) ===\n";
    InitializeCriticalSection(&g_cs);
    g_counter = 0;
    int times = 50000;

    HANDLE t1 = CreateThread(nullptr, 0, incrementFunc, &times, 0, nullptr);
    HANDLE t2 = CreateThread(nullptr, 0, incrementFunc, &times, 0, nullptr);
    WaitForSingleObject(t1, INFINITE);
    WaitForSingleObject(t2, INFINITE);
    CloseHandle(t1); CloseHandle(t2);

    std::cout << "  counter = " << g_counter << " (期望 100000)\n";
    DeleteCriticalSection(&g_cs);
    std::cout << "  C++11等价: std::lock_guard<std::mutex> lock(mtx);\n---\n";
}

// ==================== 3. Event 对象（线程同步） ====================
// Event: 简单的线程间通知机制，替代条件变量
CRITICAL_SECTION g_evCS;
HANDLE g_dataReady;
int g_sharedData = 0;

DWORD WINAPI eventProducer(LPVOID) {
    for (int i = 1; i <= 5; ++i) {
        Sleep(500);  // 模拟生产耗时
        EnterCriticalSection(&g_evCS);
        g_sharedData = i;
        std::cout << "  生产: " << i << "\n";
        LeaveCriticalSection(&g_evCS);
        SetEvent(g_dataReady);  // 通知消费者：数据就绪
    }
    return 0;
}

DWORD WINAPI eventConsumer(LPVOID) {
    for (int i = 0; i < 5; ++i) {
        WaitForSingleObject(g_dataReady, INFINITE);  // 等待数据
        EnterCriticalSection(&g_evCS);
        std::cout << "  消费: " << g_sharedData << "\n";
        LeaveCriticalSection(&g_evCS);
        ResetEvent(g_dataReady);  // 重置事件，准备下一轮
    }
    return 0;
}

void demo_event() {
    std::cout << "=== 线程同步 (Event) ===\n";
    InitializeCriticalSection(&g_evCS);
    // CreateEvent: 创建事件对象，手动重置模式
    g_dataReady = CreateEvent(nullptr, TRUE, FALSE, nullptr);

    HANDLE pt = CreateThread(nullptr, 0, eventProducer, nullptr, 0, nullptr);
    HANDLE ct = CreateThread(nullptr, 0, eventConsumer, nullptr, 0, nullptr);
    WaitForSingleObject(pt, INFINITE);
    WaitForSingleObject(ct, INFINITE);
    CloseHandle(pt); CloseHandle(ct);

    DeleteCriticalSection(&g_evCS);
    CloseHandle(g_dataReady);
    std::cout << "  C++11等价: std::condition_variable\n---\n";
}

// ==================== 4. 原子操作 ====================
LONG g_atomicCounter = 0;

DWORD WINAPI atomicFunc(LPVOID) {
    for (int i = 0; i < 50000; ++i) {
        // InterlockedIncrement: 原子递增（无需锁）
        InterlockedIncrement(&g_atomicCounter);
    }
    return 0;
}

void demo_atomic() {
    std::cout << "=== 原子操作 (Interlocked) ===\n";
    g_atomicCounter = 0;

    HANDLE threads[10];
    for (int i = 0; i < 10; ++i) {
        threads[i] = CreateThread(nullptr, 0, atomicFunc, nullptr, 0, nullptr);
    }
    // WaitForMultipleObjects: 等待多个线程全部完成
    WaitForMultipleObjects(10, threads, TRUE, INFINITE);
    for (int i = 0; i < 10; ++i) CloseHandle(threads[i]);

    std::cout << "  counter = " << g_atomicCounter << " (期望 500000)\n";
    std::cout << "  C++11等价: std::atomic<int>\n---\n";
}

int main() {
    std::cout << "=== 多线程速成示例 ===\n\n";
    demo_basic_thread();
    demo_mutex();
    demo_event();
    demo_atomic();

    std::cout << "\n=== 进阶 ===\n";
    std::cout << "生产环境推荐 C++11 标准库（需要 GCC 7+/MSVC）:\n";
    std::cout << "  std::thread              -- 线程\n";
    std::cout << "  std::mutex + lock_guard  -- 互斥锁 (RAII)\n";
    std::cout << "  std::condition_variable  -- 条件变量\n";
    std::cout << "  std::async / future      -- 异步任务\n";
    std::cout << "  std::atomic              -- 原子操作\n";
    std::cout << "\n完整 C++11 线程池: LearningAssistant/src/threading/\n";
    return 0;
}
