#ifndef COMMON_H
#define COMMON_H

#include <iostream>
#include <vector>
#include <list>
#include <functional>
#include <stdexcept>
#include <omp.h>
#include <atomic>

template<typename K>
// implement a hash function for the key K is parameter can help to define different 
struct Hash {
    size_t operator()(const K& key) const {
        return std::hash<K>{}(key);
        // make key map to integer(hash value)
    }
};

template<typename K, typename V>
struct KeyValue {
    K key;
    V value;
    
    KeyValue(const K& k, const V& v) : key(k), value(v) {}
};
#endif