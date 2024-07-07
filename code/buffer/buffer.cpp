#include "buffer.h"

// 构造函数，初始化缓冲区大小、读写指针
Buffer::Buffer(int initBuffSize) : buffer_(initBuffSize), readPos_(0), writePos_(0) {}

// 返回缓冲区中可读数据的长度
size_t Buffer::ReadableBytes() const {
    return writePos_ - readPos_;
}

// 返回缓冲区中可写的空间大小
size_t Buffer::WritableBytes() const {
    return buffer_.size() - writePos_;
}

// 返回缓冲区前端的可回收空间大小
size_t Buffer::PrependableBytes() const {
    return readPos_;
}

// 返回可读数据的起始位置的指针
const char* Buffer::Peek() const {
    return BeginPtr_() + readPos_;
}

// 根据len回收数据，即移动读指针
void Buffer::Retrieve(size_t len) {
    assert(len <= ReadableBytes()); // 确保回收的长度不超过 可读数据长度
    readPos_ += len;
}

// 回收数据直到指定的end指针位置
void Buffer::RetrieveUntil(const char* end) {
    assert(Peek() <= end);
    Retrieve(end - Peek());
}

// 清空缓冲区，重置读写指针，并清零缓冲区
void Buffer::RetrieveAll() {
    // bzero(void *s, size_t n);
    // void *s：指向要置零的内存区域的起始地址。
    // size_t n：指定了要置零的字节数。
    bzero(&buffer_[0], buffer_.size());
    // 读取位置
    readPos_ = 0;
    // 写的位置
    writePos_ = 0;
}

// 清空缓冲区，并返回之前缓冲区中的所有数据转换成的字符串
std::string Buffer::RetrieveAllToStr() {
    // 构造函数std::string(const char* s, size_t n)接受两个参数：
    //      const char* s：指向字符序列起始的指针。
    //      size_t n：序列中的字符数。
    // 这个构造函数创建一个字符串，内容是从指针s指向的位置开始的、长度为n的字符序列。
    // 这意味着，如果s指向的是一个字符数组或者内存中的一段连续字符序列，
    // 那么这个构造函数会复制从s开始的、长度为n的那部分内容到新创建的字符串中。
    std::string str(Peek(), ReadableBytes()); // 利用可读数据创建字符串
    RetrieveAll(); // 清空缓冲区
    return str;
}

// 返回写指针的const版本，即不可修改的写数据起始位置
const char* Buffer::BeginWriteConst() const {
    return BeginPtr_() + writePos_;
}

// 返回可修改的写数据起始位置的指针
char* Buffer::BeginWrite() {
    return BeginPtr_() + writePos_;
}

// 更新写指针，表示已经写入len字节的数据
void Buffer::HasWritten(size_t len) {
    writePos_ += len;
} 

// 向缓冲区追加字符串数据
void Buffer::Append(const std::string& str) {
    // std::string的data()函数返回一个指向字符串内部数据的指针
    // C++11开始，data()和c_str()方法的行为被标准化为等效的
    Append(str.data(), str.length());
}

// 将void指针指向的数据追加到缓冲区，首先确保类型转换为const char* 
void Buffer::Append(const void* data, size_t len) {
    assert(data);
    // static_cast来将某个类型的指针转换为另一个类型的指针。
    // 它将data变量的指针类型转换为指向const char的指针类型。
    // 它在编译时执行类型转换，没有运行时类型检查
    Append(static_cast<const char*>(data), len);
}

// 将char指针指向的数据追加到缓冲区
void Buffer::Append(const char* str, size_t len) {
    assert(str);
    EnsureWriteable(len); // 确保有足够空间写入数据
    std::copy(str, str + len, BeginWrite()); // 将数据复制到缓冲区的写位置
    HasWritten(len); // 更新写指针位置
}

// 将另一个Buffer对象中的数据追加到当前缓冲区
void Buffer::Append(const Buffer& buff) {
    Append(buff.Peek(), buff.ReadableBytes());
}

// 确保缓冲区有足够的空间写入指定长度的数据
void Buffer::EnsureWriteable(size_t len) {
    if(WritableBytes() < len) {
        MakeSpace_(len); // 如果可写空间不足，则调整缓冲区大小
    }
    assert(WritableBytes() >= len);
}

// 从文件描述符读取数据到缓冲区,向 缓冲区buffer中写数据，读fd。
ssize_t Buffer::ReadFd(int fd, int* saveErrno) {
    char buff[65535]; // 临时缓冲区

    // 分两个区域进行分散读，避免多次进行read，减少了系统调用的开销和上下文切换的成本。
    
    struct iovec iov[2];
    const size_t writable = WritableBytes();
    
    iov[0].iov_base = BeginPtr_() + writePos_; // 第一部分直接写入缓冲区可写位置
    iov[0].iov_len = writable; // 可写的长度

    iov[1].iov_base = buff; // 第二部分写入临时缓冲区
    iov[1].iov_len = sizeof(buff);

    // 通过一次系统调用readv完成的
    // 告诉内核从fd读取数据，首先填充到Buffer的可写空间，
    // 如果第一个缓冲区填满，剩余的数据会继续填充到临时缓冲区buff中。
    const ssize_t len = readv(fd, iov, 2); // 使用readv进行分散读

    if(len < 0) {
        *saveErrno = errno; // 读取失败，保存错误码
    }
    else if(static_cast<size_t>(len) <= writable) {
        writePos_ += len; // 全部数据都写入了缓冲区
    }
    else {
        writePos_ = buffer_.size();
        Append(buff, len - writable); // 将临时缓冲区的数据追加到缓冲区
    }
    return len;
}

// 将缓冲区的数据写入文件描述符，从缓冲区中读数据，写到fd
ssize_t Buffer::WriteFd(int fd, int* saveErrno) {

    // 获取可以读的长度
    size_t readSize = ReadableBytes();

    // 将 fd 缓冲区头的指针 可读的长度, 写到fd.
    ssize_t len = write(fd, Peek(), readSize);
    if(len < 0) {
        *saveErrno = errno; // 写入失败，保存错误码
        return len;
    } 

    readPos_ += len; // 更新读指针，表示已经将数据写出
    return len;
}

// 返回缓冲区开始的指针
char* Buffer::BeginPtr_() {
    return &*buffer_.begin();
}

// 返回缓冲区开始的const指针
const char* Buffer::BeginPtr_() const {
    return &*buffer_.begin();
}

// 当可写空间不足时，调整缓冲区大小或整理缓冲区以腾出空间
void Buffer::MakeSpace_(size_t len) {
    // 如果当前的 可写的长度 + 前面已经回收的长度，还是小于需要的长度的len话，就重新调整vector<char>的长度。
    if(WritableBytes() + PrependableBytes() < len + 1) {
        // resize方法调整vector的大小以使其能够容纳n个元素
        buffer_.resize(writePos_ + len + 1); // 直接扩展缓冲区大小
    } 
    // 将已有的可读数据向缓冲区的开始移动来腾出足够的可写空间
    else {
        size_t readable = ReadableBytes(); // 当前可读数据长度
        // 将缓冲区中的可读数据（位于readPos_和writePos_之间的数据）移动到缓冲区的开始位置。
        // 这样做的目的通常是为了回收缓冲区前面的空间，使得后续写入操作有更多的连续空间可用
        std::copy(BeginPtr_() + readPos_, BeginPtr_() + writePos_, BeginPtr_()); // 将可读数据前移
        readPos_ = 0; // 重置读指针
        writePos_ = readable; // 更新写指针
        assert(readable == ReadableBytes()); // 确保可读数据长度未变
    }
}