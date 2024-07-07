/*
 * @Author       : mark
 * @Date         : 2020-06-15
 * @copyleft Apache 2.0
 */ 

#ifndef THREADPOOL_H // 如果 THREADPOOL_H 未定义（防止头文件被重复包含）
#define THREADPOOL_H // 定义 THREADPOOL_H

#include <mutex> // 包含互斥量相关的头文件
#include <condition_variable> // 包含条件变量相关的头文件
#include <queue> // 包含队列相关的头文件
#include <thread> // 包含线程相关的头文件
#include <functional> // 包含函数对象相关的头文件

class ThreadPool { // 线程池类的声明开始
public: // 公有成员部分

    explicit ThreadPool(size_t threadCount = 8) // 构造函数，指定默认线程数量为8
        : pool_(std::make_shared<Pool>()) { // 初始化线程池对象，使用std::make_shared创建Pool结构体的智能指针
        assert(threadCount > 0); // 断言，确保线程数量大于0

        // 创建threadCount个子线程
        for (size_t i = 0; i < threadCount; i++) { // 循环创建指定数量的子线程
            
            std::thread([pool = pool_] { // 创建一个子线程，并捕获线程池的共享指针
                std::unique_lock<std::mutex> locker(pool->mtx); // 创建互斥锁，并锁定互斥量
                while (true) { // 无限循环
                    if (!pool->tasks.empty()) { // 如果任务队列不为空
                        auto task = std::move(pool->tasks.front()); // 获取队列首个任务，并移动到局部变量中
                        pool->tasks.pop(); // 从队列中移除任务
                        locker.unlock(); // 解锁互斥量
                        task(); // 执行任务
                        locker.lock(); // 再次锁定互斥量
                    } else if (pool->isClosed) { // 如果任务队列为空且线程池已关闭
                        break; // 退出循环
                    } else {
                        pool->cond.wait(locker); // 使用条件变量等待通知
                    }
                }
            }
            
            ).detach(); // 设置线程为分离状态，使其在结束时自动释放资源
        }
    }

    ThreadPool() = default; // 默认构造函数

    ThreadPool(ThreadPool&&) = default; // 移动构造函数

    ~ThreadPool() { // 析构函数
        if (static_cast<bool>(pool_)) { // 如果线程池对象存在
            { // 开始临界区域
                std::lock_guard<std::mutex> locker(pool_->mtx); // 创建互斥量锁定对象，自动释放锁
                pool_->isClosed = true; // 设置线程池已关闭标志为true
            } // 结束临界区域
            pool_->cond.notify_all(); // 唤醒所有等待的线程
        }
    }

    template<class F> // 模板函数，用于添加任务到线程池
    void AddTask(F&& task) { // 接受一个可调用对象作为参数
        {
            std::lock_guard<std::mutex> locker(pool_->mtx); // 创建互斥量锁定对象，自动释放锁
            pool_->tasks.emplace(std::forward<F>(task)); // 将任务添加到任务队列中
        }
        pool_->cond.notify_one(); // 唤醒一个等待的线程
    }

private: // 私有成员部分

    struct Pool { // 内部结构体 Pool，用于管理线程池的相关信息
        std::mutex mtx; // 互斥锁，保护线程池相关数据结构
        std::condition_variable cond; // 条件变量，用于线程之间的同步
        bool isClosed; // 标志位，表示线程池是否关闭
        std::queue<std::function<void()>> tasks; // 任务队列，保存待执行的任务
    };

    std::shared_ptr<Pool> pool_; // 指向 Pool 结构体的智能指针，用于管理线程池的生命周期
};

#endif //THREADPOOL_H // 结束头文件保护宏的定义
