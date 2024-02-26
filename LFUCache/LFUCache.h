#pragma once
#define LFU_CAPACITY 10
#include <iostream>
#include <mutex>
#include <unordered_map>

namespace Lfu {
    using std::string;

    //链表的节点
    template<typename T>
    class Node {
    public:
        void setPre(Node* p) { pre_ = p; }
        void setNext(Node* p) { next_ = p; }
        Node* getPre() { return pre_; }
        Node* getNext() { return next_; }
        T& getValue() { return value_; }
    private:
        T value_;
        Node* pre_;
        Node* next_;
    };

    //文件名->文件内容映射
    struct Key {
        string key_, value_;
    };

    using keyNode = Node<Key>*;

    //链表：由多个Node组成
    class KeyList {
    public:
        void init(int freq);
        void destroy();
        int getFreq();
        void add(keyNode& node);
        void del(keyNode& node);
        bool isEmpty();
        keyNode getLast();
    
    private:
        int freq_;
        keyNode dummyHead_;
        keyNode tail_;

    };

    using freqNode = Node<KeyList>*;

    /*
    典型的双重链表+hash表实现
    ⾸先是⼀个⼤链表，链表的每个节点都是⼀个⼩链表附带⼀个值表示频度
    ⼩链表存的是同⼀频度下的key value节点。
    小链表从前往后按照LRU算法越往后越久没访问
    大链表从前往后是默认按频度升序，是升序链表
    hash表存key到⼤链表节点的映射(key,freq_node)和
    key到⼩链表节点的映射(key,key_node).
    */

    //LFU由多个链表组成
    class LFUCache {
        freqNode dummyHead_; // ⼤链表的头节点，⾥⾯每个节点都是⼩链表的头节点
        size_t capacity_;
        std::mutex mutex_;
        std::unordered_map<string, keyNode> kmap_; // key到keyNode的映射
        std::unordered_map<string, freqNode> fmap_; // key到freqNode的映射
        void addFreq(keyNode& nowk, freqNode& nowf);
        void del(freqNode& node);
        void init();
    public:
        LFUCache(int capicity);
        ~LFUCache();
        bool get(string& key, string& value); // 通过key返回value并进⾏LFU操作
        void set(string& key, string& value); // 更新LFU缓存
        size_t getCapacity() const { return capacity_; }
    };

    LFUCache& getCache();
}

