#ifndef HEAP_TIMER_H // 如果未定义 HEAP_TIMER_H，防止头文件被重复包含
#define HEAP_TIMER_H // 定义 HEAP_TIMER_H

#include <queue> // 包含队列的头文件
#include <unordered_map> // 包含无序映射的头文件
#include <time.h> // 包含时间相关的头文件
#include <algorithm> // 包含算法相关的头文件
#include <arpa/inet.h> // 包含网络相关的头文件
#include <functional> // 包含函数对象相关的头文件
#include <assert.h> // 包含断言相关的头文件
#include <chrono> // 包含时间库的头文件
#include "../log/log.h" // 包含日志相关的头文件

// 定义函数对象类型 TimeoutCallBack、时间点类型 TimeStamp、时间间隔类型 MS
typedef std::function<void()> TimeoutCallBack;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;
typedef Clock::time_point TimeStamp;

// 定义结构体 TimerNode，表示计时器节点
struct TimerNode {
    int id; // 计时器节点的ID
    TimeStamp expires; // 计时器节点的到期时间
    TimeoutCallBack cb; // 计时器节点的回调函数

    // 重载小于运算符，用于比较计时器节点的到期时间
    bool operator<(const TimerNode& t) {
        return expires < t.expires;
    }
};

// 定义 HeapTimer 类，用于管理计时器节点
class HeapTimer {
public: // 公有成员部分

    HeapTimer() { heap_.reserve(64); } // 构造函数，初始化堆容量为64

    ~HeapTimer() { clear(); } // 析构函数，释放堆资源

    void adjust(int id, int newExpires); // 调整计时器节点到期时间

    void add(int id, int timeOut, const TimeoutCallBack& cb); // 添加计时器节点

    void doWork(int id); // 处理计时器节点的工作

    void clear(); // 清空堆

    void tick(); // 触发超时计时器节点的回调函数

    void pop(); // 弹出堆顶元素

    int GetNextTick(); // 获取下一个计时器的超时时间

private: // 私有成员部分

    void del_(size_t i); // 删除指定位置的计时器节点

    void siftup_(size_t i); // 上浮操作，用于调整堆结构

    bool siftdown_(size_t index, size_t n); // 下沉操作，用于调整堆结构

    void SwapNode_(size_t i, size_t j); // 交换两个计时器节点

    std::vector<TimerNode> heap_; // 用于存储计时器节点的堆

    std::unordered_map<int, size_t> ref_; // 记录计时器节点ID到在堆中的位置的映射
};

#endif //HEAP_TIMER_H // 结束头文件保护宏的定义
