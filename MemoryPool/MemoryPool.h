#pragma once
#include <mutex>

namespace MPool {
    #define BlockSize 4096

    struct Slot {
        Slot* next;
    };

    class MemoryPool {
    public:
        MemoryPool() = default;
        ~MemoryPool();

        void init(int size);
        //分配或收回一个元素的内存空间
        Slot* allocate();  
        void deAllocate(Slot* p);
    private:
        int slotSize_;  //每个槽所占字节数
        Slot* currentBlock_;    //内存块链表头指针
        Slot* currentSlot_; //元素链表头指针
        Slot* lastSlot_;    //可存放元素最后指针
        Slot* freeSlot_;    //元素构造后释放掉内存链表头指针

        std::mutex mutexFreeSlot_;  //访问释放内存链表
        std::mutex mutexOther_;
        size_t padPointer(char* p, size_t align);       //计算对齐所需空间
        Slot* allocateBlock();  //申请内存块放进内存池
        Slot* nofreeSolve();
    };

    void initMemoryPool();
    void* useMemory(size_t size);
    void freeMemory(size_t size, void* p);
    MemoryPool& getMemoryPool(int id);

    template<typename T, typename... Args>
    T* newElement(Args&&... args) {
        T* p;
        if ((p = reinterpret_cast<T*>(useMemory(sizeof(T)))) != nullptr)
            //new(p) T1(value);
            //placement new:在指针p所指向内存空间创建一个T1类型的对象
            //把已有空间当成缓冲区用，减少分配空间所耗费时间
            //因为直接用new分配内存的话，在堆中查找足够大剩余空间速度是比较慢的
            new(p) T(std::forward<Args>(args)...);  //完美转发
        return p;
    }

    template<typename T>
    void deleteElement(T* p) {
        if (p) p->~T();
        freeMemory(sizeof(T), reinterpret_cast<void*>(p));
    }
};
