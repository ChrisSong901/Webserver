#include "heaptimer.h" // 包含头文件 "heaptimer.h"

// 上浮操作，用于调整堆结构
void HeapTimer::siftup_(size_t i) {
    assert(i >= 0 && i < heap_.size()); // 断言，确保下标 i 合法
    size_t j = (i - 1) / 2; // 计算父节点的下标
    while(j >= 0) { // 循环直到根节点
        if(heap_[j] < heap_[i]) { break; } // 如果父节点的到期时间小于子节点的到期时间，则不需要调整
        SwapNode_(i, j); // 否则交换父子节点
        i = j; // 更新当前节点下标
        j = (i - 1) / 2; // 更新父节点下标
    }
}

// 交换两个计时器节点
void HeapTimer::SwapNode_(size_t i, size_t j) {
    assert(i >= 0 && i < heap_.size()); // 断言，确保下标 i 合法
    assert(j >= 0 && j < heap_.size()); // 断言，确保下标 j 合法
    std::swap(heap_[i], heap_[j]); // 交换两个节点
    ref_[heap_[i].id] = i; // 更新交换后的节点在映射表中的位置
    ref_[heap_[j].id] = j; // 更新交换后的节点在映射表中的位置
} 

// 下沉操作，用于调整堆结构
bool HeapTimer::siftdown_(size_t index, size_t n) {
    assert(index >= 0 && index < heap_.size()); // 断言，确保下标 index 合法
    assert(n >= 0 && n <= heap_.size()); // 断言，确保范围 [0, n] 合法
    size_t i = index; // 当前节点下标
    size_t j = i * 2 + 1; // 左子节点下标
    while(j < n) { // 循环直到叶子节点
        if(j + 1 < n && heap_[j + 1] < heap_[j]) j++; // 如果右子节点存在且比左子节点小，则选择右子节点
        if(heap_[i] < heap_[j]) break; // 如果当前节点小于等于子节点，则不需要调整
        SwapNode_(i, j); // 否则交换当前节点和子节点
        i = j; // 更新当前节点下标
        j = i * 2 + 1; // 更新子节点下标
    }
    return i > index; // 返回是否进行了调整
}

// 添加计时器节点
void HeapTimer::add(int id, int timeout, const TimeoutCallBack& cb) {
    assert(id >= 0); // 断言，确保计时器节点的ID合法
    size_t i;
    if(ref_.count(id) == 0) { // 如果计时器节点不存在
        /* 新节点：堆尾插入，调整堆 */
        i = heap_.size(); // 新节点的位置为堆的末尾
        ref_[id] = i; // 更新映射表
        heap_.push_back({id, Clock::now() + MS(timeout), cb}); // 在堆末尾插入新节点
        siftup_(i); // 调整堆结构
    } 
    else {
        /* 已有结点：调整堆 */
        i = ref_[id]; // 获取已有节点的位置
        heap_[i].expires = Clock::now() + MS(timeout); // 更新节点的到期时间
        heap_[i].cb = cb; // 更新节点的回调函数
        if(!siftdown_(i, heap_.size())) { // 调整堆结构
            siftup_(i);
        }
    }
}

// 处理计时器节点的工作
void HeapTimer::doWork(int id) {
    /* 删除指定id结点，并触发回调函数 */
    if(heap_.empty() || ref_.count(id) == 0) {
        return;
    }
    size_t i = ref_[id]; // 获取计时器节点的位置
    TimerNode node = heap_[i]; // 获取计时器节点
    node.cb(); // 调用回调函数
    del_(i); // 删除计时器节点
}

// 删除指定位置的计时器节点
void HeapTimer::del_(size_t index) {
    /* 删除指定位置的结点 */
    assert(!heap_.empty() && index >= 0 && index < heap_.size()); // 断言，确保堆不为空且下标合法
    /* 将要删除的结点换到队尾，然后调整堆 */
    size_t i = index; // 获取要删除节点的位置
    size_t n = heap_.size() - 1; // 获取堆的最后一个节点位置
    assert(i <= n); // 断言，确保要删除的节点位置不超过堆的范围
    if(i < n) { // 如果要删除的节点不是堆的最后一个节点
        SwapNode_(i, n); // 将要删除的节点与堆的最后一个节点交换
        if(!siftdown_(i, n)) { // 调整堆结构
            siftup_(i);
        }
    }
    /* 队尾元素删除 */
    ref_.erase(heap_.back().id); // 从映射表中删除最后一个节点的ID
    heap_.pop_back(); // 删除最后一个节点
}

// 调整指定id的结点
void HeapTimer::adjust(int id, int timeout) {
    /* 调整指定id的结点 */
    assert(!heap_.empty() && ref_.count(id) > 0); // 断言，确保堆不为空且指定ID存在于映射表中
    heap_[ref_[id]].expires = Clock::now() + MS(timeout);; // 更新节点的到期时间
    siftdown_(ref_[id], heap_.size()); // 调整堆结构
}

// 触发超时计时器节点的回调函数
void HeapTimer::tick() {
    /* 清除超时结点 */
    if(heap_.empty()) { // 如果堆为空，直接返回
        return;
    }
    while(!heap_.empty()) { // 循环直到堆为空
        TimerNode node = heap_.front(); // 获取堆顶元素
        if(std::chrono::duration_cast<MS>(node.expires - Clock::now()).count() > 0) { // 如果堆顶元素的到期时间未到
            break; // 跳出循环
        }
        node.cb(); // 调用回调函数
        pop(); // 弹出堆顶元素
    }
}

// 弹出堆顶元素
void HeapTimer::pop() {
    assert(!heap_.empty()); // 断言，确保堆不为空
    del_(0); // 删除堆顶元素
}

// 清空堆
void HeapTimer::clear() {
    ref_.clear(); // 清空映射表
    heap_.clear(); // 清空堆
}

// 获取下一个计时器的超时时间
int HeapTimer::GetNextTick() {
    tick(); // 触发超时计时器节点的回调函数
    size_t res = -1; // 初始化返回值为-1
    if(!heap_.empty()) { // 如果堆不为空
        res = std::chrono::duration_cast<MS>(heap_.front().expires - Clock::now()).count(); // 计算堆顶元素的超时时间
        if(res < 0) { res = 0; } // 如果超时时间为负数，则设置为0
    }
    return res; // 返回超时时间
}
