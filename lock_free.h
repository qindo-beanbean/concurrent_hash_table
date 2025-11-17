#ifndef LOCK_FREE_H
#define LOCK_FREE_H

#include "common.h"
#include <atomic>
#include <memory>

template<typename K, typename V>
class LockFreeHashTable {
private:
    struct Node {
        K key;
        V value;
        std::atomic<Node*> next;
        
        Node(const K& k, const V& v) : key(k), value(v), next(nullptr) {}
    };
    
    struct Bucket {
        std::atomic<Node*> head;
        Bucket() : head(nullptr) {}
        // Move constructor
        Bucket(Bucket&& other) noexcept : head(other.head.load()) {}
        // Move assignment operator
        Bucket& operator=(Bucket&& other) noexcept {
            head.store(other.head.load());
            return *this;
        }
        // Delete copy constructor and copy assignment (atomic is not copyable)
        Bucket(const Bucket&) = delete;
        Bucket& operator=(const Bucket&) = delete;
    };
    
    std::vector<Bucket> buckets;
    size_t bucket_count;
    std::atomic<size_t> element_count;
    
    size_t hash(const K& key) const {
        return Hash<K>{}(key) % bucket_count;
    }

public:
    LockFreeHashTable(size_t bucket_count = 1024) 
        : bucket_count(bucket_count), element_count(0) {
        buckets.reserve(bucket_count);
        for (size_t i = 0; i < bucket_count; ++i) {
            buckets.emplace_back();
        }
    }
    
    ~LockFreeHashTable() {
        for (size_t i = 0; i < bucket_count; ++i) {
            Node* current = buckets[i].head.load();
            while (current) {
                Node* next = current->next.load();
                delete current;
                current = next;
            }
        }
    }
    
    bool insert(const K& key, const V& value) {


        size_t idx = hash(key);
        Node* new_node = new Node(key, value);
        
        while (true) {
            Node* head = buckets[idx].head.load();
            
            // Check if key already exists
            Node* current = head;
            while (current) {
                if (current->key == key) {
                    // Key exists, update value (simplified version, should use CAS in practice)
                    current->value = value;
                    delete new_node;
                    return false;
                }
                current = current->next.load();
            }
            
            // Try to insert at the head of the list
            new_node->next.store(head);
            if (buckets[idx].head.compare_exchange_weak(head, new_node)) {
                element_count++;
                return true;
            }
            // CAS failed, retry
        }
    }
    
    bool search(const K& key, V& value) const {
        size_t idx = hash(key);
        Node* current = buckets[idx].head.load();
        
        while (current) {
            if (current->key == key) {
                value = current->value;
                return true;
            }
            current = current->next.load();
        }
        
        return false;
    }
    
    bool remove(const K& key) {
        size_t idx = hash(key);
        
        while (true) {
            Node* head = buckets[idx].head.load();
            Node* current = head;
            Node* prev = nullptr;
            
            // Find the node to delete
            while (current) {
                if (current->key == key) {
                    if (prev == nullptr) {
                        // Delete head node
                        if (buckets[idx].head.compare_exchange_weak(head, current->next.load())) {
                            delete current;
                            element_count--;
                            return true;
                        }
                    } else {
                        // Delete middle node (simplified version)
                        prev->next.store(current->next.load());
                        delete current;
                        element_count--;
                        return true;
                    }
                    break;  // Retry
                }
                prev = current;
                current = current->next.load();
            }
            
            if (!current) return false;  // Not found
        }
    }
    
    size_t size() const {
        return element_count.load();
    }
    
    std::string getName() const {
        return "Lock-Free";
    }
};

#endif // LOCK_FREE_H