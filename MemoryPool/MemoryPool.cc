#include <assert.h>
#include "MemoryPool.h"

using namespace MPool;

MemoryPool::~MemoryPool() {
    Slot* cur = currentBlock_;
    while (cur) {
        Slot* next = cur->next;
        //转化为void* 指针，是因为void类型不需要调用析构函数，只释放空间
        operator delete(reinterpret_cast<void*>(cur));
        cur = next;
    }
}

void MemoryPool::init(int size) {
    assert(size > 0);
    slotSize_ = size;
    currentBlock_ = nullptr;
    currentSlot_ = nullptr;
    lastSlot_ = nullptr;
    freeSlot_ = nullptr;
}

//计算对齐所需补的空间
inline size_t MemoryPool::padPointer(char* p, size_t align) {
    size_t result = reinterpret_cast<size_t>(p);
    return ((align - result) % align);
}

//申请内存池分配内存块
Slot* MemoryPool::allocateBlock() {
    char* newBlock = reinterpret_cast<char*>(operator new(BlockSize));

    char* body = newBlock + sizeof(Slot*);
    //计算为了对齐需要空出多少位置
    size_t bodyPadding = padPointer(body, static_cast<size_t>(slotSize_));
    
    //多个线程(EventLoop 共用一个MemoryPool)
    Slot* useSlot;
    {
        std::unique_lock<std::mutex> lk(mutexOther_);
        //newBlock接到Block链表头部
        reinterpret_cast<Slot*>(newBlock)->next = currentBlock_;
        currentBlock_ = reinterpret_cast<Slot*>(newBlock);
        //为该Block开始的地方加上bodyPadding个char* 空间
        currentSlot_ = reinterpret_cast<Slot*>(body + bodyPadding);
        lastSlot_ = reinterpret_cast<Slot*>(newBlock + BlockSize - slotSize_ + 1);
        useSlot = currentSlot_;
        //slot指针一次移动8字节
        currentSlot_ += (slotSize_ >> 3);
    }

    return useSlot;
}

//没有空闲空间方法
Slot* MemoryPool::nofreeSolve() {
    if (currentSlot_ >= lastSlot_) return allocateBlock();
    Slot* useSlot;
    {
        std::unique_lock<std::mutex> lk(mutexOther_);
        useSlot = currentSlot_;
        currentSlot_ += (slotSize_ >> 3);
    }
    return useSlot;
}

//分配一个槽空间
Slot* MemoryPool::allocate() {
    if (freeSlot_) {
        {
            std::unique_lock<std::mutex> lk(mutexFreeSlot_);
            if (freeSlot_) {
                Slot* result = freeSlot_;
                freeSlot_ = freeSlot_->next;
                return result;
            }
        }
    }

    return nofreeSolve();
}

inline void MemoryPool::deAllocate(Slot* p) {
    if (p) {
        std::unique_lock<std::mutex> lk(mutexFreeSlot_);
        p->next = freeSlot_;
        freeSlot_ = p;
    }
}

MemoryPool& MPool::getMemoryPool(int id) {
    static MemoryPool memorypool_[64];
    return memorypool_[id];
}

//数组中分别存放Slot为8,16,24...,512字节的Block链表
void MPool::initMemoryPool() {
    for (int i = 0; i < 64; i++) {
        getMemoryPool(i).init((i + 1) << 3);
    }
}

//超过512字节就直接new
void* MPool::useMemory(size_t size) {
    if (!size) return nullptr;
    if (size > 512) return operator new(size);

    //相当于size / 8向上取整
    return reinterpret_cast<void*>(getMemoryPool(((size + 7) >> 3) - 1).allocate());
}

void MPool::freeMemory(size_t size, void* p) {
    if (!p) return;
    if (size > 512) {
        operator delete(p);
        return;
    }
    getMemoryPool(((size + 7) >> 3) - 1).deAllocate(reinterpret_cast<Slot*>(p));
}


