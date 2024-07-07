#ifndef BUFFER_H
#define BUFFER_H

#include <cstring>   // 包含C语言的字符串处理函数
#include <iostream>  // 提供输入输出流的功能
#include <unistd.h>  // 提供UNIX标准的函数定义，如write()
#include <sys/uio.h> // 提供readv()和writev()函数的定义
#include <vector>    // 使用vector作为缓冲区的底层实现
#include <atomic>    // 提供原子操作，用于线程安全的读写指针操作
#include <assert.h>  // 提供断言，用于调试中检查逻辑错误

class Buffer {
public:
    // 构造函数，默认初始化缓冲区大小为1024字节
    Buffer(int initBuffSize = 1024);
    // 默认的析构函数
    ~Buffer() = default;
    
    // 返回缓冲区中可写的字节数
    size_t WritableBytes() const;       
    // 返回缓冲区中可读的字节数
    size_t ReadableBytes() const ;
    // 返回缓冲区前端可回收的字节数
    size_t PrependableBytes() const;

    // 返回可读数据的起始指针
    const char* Peek() const;
    // 确保有足够的空间可写入指定长度的数据，如有必要则调整缓冲区大小
    void EnsureWriteable(size_t len);
    // 更新写指针，表示已经写入了len字节的数据
    void HasWritten(size_t len);

    // 回收len长度的数据，即移动读指针
    void Retrieve(size_t len);
    // 回收数据直到指定的end指针位置
    void RetrieveUntil(const char* end);

    // 清空缓冲区
    void RetrieveAll() ;
    // 清空缓冲区，并返回之前缓冲区中的所有数据转换成的字符串
    std::string RetrieveAllToStr();

    // 返回写指针的const版本
    const char* BeginWriteConst() const;
    // 返回写指针
    char* BeginWrite();

    // 向缓冲区追加数据的方法
    void Append(const std::string& str);
    void Append(const char* str, size_t len);
    void Append(const void* data, size_t len);
    void Append(const Buffer& buff);

    // 从文件描述符读取数据到缓冲区
    ssize_t ReadFd(int fd, int* Errno);
    // 将缓冲区的数据写入文件描述符
    ssize_t WriteFd(int fd, int* Errno);

private:
    // 获取缓冲区开始的指针的方法
    char* BeginPtr_();
    const char* BeginPtr_() const;
    // 当可写空间不足时，调整缓冲区大小或整理缓冲区以腾出空间
    void MakeSpace_(size_t len);

    std::vector<char> buffer_; // 实际存储数据的vector
    std::atomic<std::size_t> readPos_; // 缓冲区中的读指针位置
    std::atomic<std::size_t> writePos_; // 缓冲区中的写指针位置
};

#endif //BUFFER_H
