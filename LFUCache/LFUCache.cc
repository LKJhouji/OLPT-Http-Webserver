#include <assert.h>
#include "LFUCache.h"
#include "../MemoryPool/MemoryPool.h"

using namespace Lfu;
using namespace MPool;

void KeyList::init(int freq) {
    freq_ = freq;
    // Dummyhead_ = tail_= new Node<Key>;
    dummyHead_ = newElement<Node<Key>>();
    tail_ = dummyHead_;
    dummyHead_->setNext(nullptr);
}

// 删除整个链表
void KeyList::destroy() {
    while(dummyHead_ != nullptr) {
    keyNode pre = dummyHead_;
    dummyHead_ = dummyHead_->getNext();
    // delete pre;
    deleteElement(pre);
    }
}

int KeyList::getFreq() { return freq_; }

// 将节点添加到链表头部
void KeyList::add(keyNode& node) {
    if(dummyHead_->getNext() == nullptr) {
        tail_ = node;
    }
    else {
        dummyHead_->getNext()->setPre(node);
    }
    node->setNext(dummyHead_->getNext());
    node->setPre(dummyHead_);
    dummyHead_->setNext(node);
    assert(!isEmpty());
}

// 删除⼩链表中的节点
void KeyList::del(keyNode& node) {
    node->getPre()->setNext(node->getNext());
    if(node->getNext() == nullptr) {
        tail_ = node->getPre();
    }
    else {
        node->getNext()->setPre(node->getPre());
    }
}

bool KeyList::isEmpty() {
    return dummyHead_ == tail_;
}
keyNode KeyList::getLast() {
    return tail_;
}
LFUCache::LFUCache(int capacity) : capacity_(capacity) {
    init();
}

LFUCache::~LFUCache() {
    while(dummyHead_) {
        freqNode pre = dummyHead_;
        dummyHead_ = dummyHead_->getNext();
        pre->getValue().destroy();
        // delete pre;
        deleteElement(pre);
    }
}

void LFUCache::init() {
    // FIXME:缓存的容量动态变化
    
    // dummyHead_ = new Node<KeyList>();
    dummyHead_ = newElement<Node<KeyList>>();
    dummyHead_->getValue().init(0);
    dummyHead_->setNext(nullptr);
}

// 更新节点频度：
// 如果不存在下⼀个频度的链表，则增加⼀个
// 然后将当前节点放到下⼀个频度的链表的头位置
void LFUCache::addFreq(keyNode& nowk, freqNode& nowf) {
    freqNode nxt;
    // FIXME:频数有可能有溢出
    if(nowf->getNext() == nullptr ||
    nowf->getNext()->getValue().getFreq() != nowf->getValue().getFreq() + 1) {
        // 新建⼀个下⼀频度的小链表,加到nowf后⾯
        // nxt = new Node<KeyList>();
        nxt = newElement<Node<KeyList>>();
        nxt->getValue().init(nowf->getValue().getFreq() + 1);
        if(nowf->getNext() != nullptr)
            nowf->getNext()->setPre(nxt);
        nxt->setNext(nowf->getNext());
        nowf->setNext(nxt);
        nxt->setPre(nowf);
    }
    else {
        nxt = nowf->getNext();
    }
    fmap_[nowk->getValue().key_] = nxt;
    // 将其从上⼀频度的⼩链表删除
    // 然后加到下⼀频度的⼩链表中
    if(nowf != dummyHead_) {
        nowf->getValue().del(nowk);
    }
    nxt->getValue().add(nowk);
    assert(!nxt->getValue().isEmpty());
    // 如果该频度的⼩链表已经空了
    if(nowf != dummyHead_ && nowf->getValue().isEmpty())
        del(nowf);
}


bool LFUCache::get(string& key, string& val) {
    if(!capacity_) return false;
    std::unique_lock<std::mutex> lk(mutex_);
    if(fmap_.find(key) != fmap_.end()) {
        // 缓存命中
        keyNode nowk = kmap_[key];
        freqNode nowf = fmap_[key];
        val += nowk->getValue().value_;
        addFreq(nowk, nowf);    //访问次数 + 1
        return true;
    }
    // 未命中
    return false;
}

void LFUCache::set(string& key, string& val) {
    if(!capacity_) return;
    std::unique_lock<std::mutex> lk(mutex_);
    // 缓存满了
    // 从频度最⼩的⼩链表中的节点中删除最后⼀个节点（⼩链表中的删除符合LRU）
    if(kmap_.size() == capacity_) {
        freqNode head = dummyHead_->getNext();
        keyNode last = head->getValue().getLast();
        head->getValue().del(last);
        kmap_.erase(last->getValue().key_);
        fmap_.erase(last->getValue().key_);
        // delete last;
        deleteElement(last);
        // 如果频度最⼩的链表已经没有节点，就删除
        if(head->getValue().isEmpty()) {
            del(head);
        }
    }
    // keyNode nowk = new Node<Key>();
    // 使⽤内存池
    keyNode nowk = newElement<Node<Key>>();
    
    nowk->getValue().key_ = key;
    nowk->getValue().value_ = val;
    addFreq(nowk, dummyHead_);
    kmap_[key] = nowk;
    fmap_[key] = dummyHead_->getNext();
}

void LFUCache::del(freqNode& node) {
    node->getPre()->setNext(node->getNext());
    if(node->getNext() != nullptr) {
    node->getNext()->setPre(node->getPre());
    }
    node->getValue().destroy();
    // delete node;
    deleteElement(node);
 
}
LFUCache& Lfu::getCache() {
    static LFUCache cache(LFU_CAPACITY);
    return cache;
}