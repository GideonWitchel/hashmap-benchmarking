#pragma once

#include "HashTable.h"
#include "Product.h"
#include "murmur3.h"

#include <iostream>
#include <vector>
#include <cmath>
#include <string>

#define DEFAULT_OPEN_MAP_SIZE 4

template<typename Key, typename Value>
class OpenMap: public HashTable<Key, Value> {
public:
    OpenMap() {
        hashmap_.resize(DEFAULT_OPEN_MAP_SIZE);
        significancemap_.resize(DEFAULT_OPEN_MAP_SIZE, false);
    }

    OpenMap(double load) {
        if(load <= 0 || load >= 1){
            std::cerr << "Invalid load factor for OpenGraph (not 0..1) - setting to 0.7 - ignore verbose load factor messaging\n";
            MAX_LOAD_FACTOR_ = 0.7;
        }
        else{
            MAX_LOAD_FACTOR_ = load;
        }
        hashmap_.resize(DEFAULT_OPEN_MAP_SIZE);
        significancemap_.resize(DEFAULT_OPEN_MAP_SIZE, false);
    }

    Value get(Key k) override {
        uint64_t index = hash(k, hashmap_.size());
        uint64_t origIndex = index;
        while(significancemap_[index]){
            if(hashmap_[index].valid_ && k == hashmap_[index].key_){
                return hashmap_[index].value_;
            }
            index = iterate(index, hashmap_.size());
            if(index == origIndex){
                std::cerr << "Open Map is full\n";
                break;
            }
        }
        return Value{};
    }

    void put(Key k, Value v) override {
        uint64_t index = hash(k, hashmap_.size());
        uint64_t origIndex = index;
        while(significancemap_[index]){
            if(!(hashmap_[index].valid_)){
                break;
            }
            else if(k == hashmap_[index].key_){
                hashmap_[index].value_ = v;
                return;
            }
            index = iterate(index, hashmap_.size());
            if(index == origIndex){
                std::cerr << "Open Map is full\n";
                return;
            }
        }
        KeyValuePair kvp(k, v);
        hashmap_[index] = kvp;
        significancemap_[index] = true;
        ++numPairs_;
        ++numSignificant_;
        if(getLoadFactor() > MAX_LOAD_FACTOR_){
            doubleSize();
        }
    }

    void remove(Key k) override {
        uint64_t index = hash(k, hashmap_.size());
        uint64_t origIndex = index;
        while(significancemap_[index]){
            if(hashmap_[index].valid_ && k == hashmap_[index].key_){
                hashmap_[index] = KeyValuePair();
                --numPairs_;
                return;
            }
            index = iterate(index, hashmap_.size());
            if(index == origIndex){
                std::cerr << "Open Map is full and does not contain the key \"" << k << "\" so it could not be removed\n";
                return;
            }
        }
        std::cerr << "Open Map does not contain the key \"" << k << "\" so it could not be removed\n";
    }

    void display() override {
        for(uint64_t i = 0; i < hashmap_.size(); ++i){
            if(hashmap_[i].valid_) {
                    std::cout << i << ": {" << ((significancemap_[i]) ? "T" : "F") << "} |" << hash(hashmap_[i].key_, hashmap_.size()) << "| ";
                    printKeyValuePair(hashmap_[i]);
                    std::cout << "\n";
            }
            else {
                    std::cout << i << ": {" << ((significancemap_[i]) ? "T" : "F") << "}\n";
            }
        }
        std::cout << std::endl;
    }

    void stats() override {
        //I'm not sure how to measure collisions in this graph type without keeping track of a variable as I add and remove pairs
        uint64_t numElements = 0;
        for(uint64_t i = 0; i < hashmap_.size(); ++i){
            if(hashmap_[i].valid_) {
                ++numElements;
            }
        }
        std::cout << "Total Elements: "<< numElements << "\n";
        std::cout << "Map Size: " << hashmap_.size() << "\n";
        std::cout << "Load Factor: " << getLoadFactor() << "\n";
        std::cout << std::endl;
    }

    double getLoadFactor() override {
        return (static_cast<float>(numSignificant_))/(static_cast<float>(hashmap_.size()));
    }

    uint64_t getFullSize() override {
        return hashmap_.size();
    }

    uint64_t getNumPairs() override {
        return numPairs_;
    }

private:
    //default max load factor before doubling size is 0.7
    double MAX_LOAD_FACTOR_ = 0.7;

    struct KeyValuePair{
        KeyValuePair(Key k, Value v) : key_(k), value_(v), valid_(true) {}
        KeyValuePair() : key_(Key{}), value_(Value{}), valid_(false) {}

        Key key_;
        Value value_;
        bool valid_;
    };

    std::vector<KeyValuePair> hashmap_;
    std::vector<bool> significancemap_;

    uint64_t numPairs_ = 0;
    uint64_t numSignificant_ = 0;

    std::hash<Key> hasher_;

    uint64_t hash(Key k, uint64_t m){
        return hasher_(k) % m;
    }

    //this is the operation done on any index that collides
    uint64_t iterate(uint64_t index, uint64_t m){
        return (index+1) % m;
    }


    void doubleSize(){
        std::vector<KeyValuePair> newMap(hashmap_.size()*2);
        std::vector<bool> newSignificance(hashmap_.size()*2, false);
        for(const auto& kvp : hashmap_){
            if(kvp.valid_){
                uint64_t index = hash(kvp.key_, newMap.size());
                while(newSignificance[index]){
                    index = iterate(index, newMap.size());
                }
                newMap[index] = kvp;
                newSignificance[index] = true;
            }
        }
        numSignificant_ = numPairs_;

        hashmap_ = newMap;
        significancemap_ = newSignificance;
    }

    void printKeyValuePair(KeyValuePair kvp){
        std::cout << " Invalid types for printing ";
    }

};

template<>
uint64_t OpenMap<uint64_t, Product*>::hash(uint64_t k, uint64_t m) {
    uint64_t out[2];
    MurmurHash3_x64_128(&k, sizeof(k), 123, out);
    return out[1] % m;
}

template<>
uint64_t OpenMap<uint64_t, uint64_t>::hash(uint64_t k, uint64_t m) {
    uint64_t out[2];
    MurmurHash3_x64_128(&k, sizeof(k), 123, out);
    return out[1] % m;
}

template<>
void OpenMap<std::string, uint64_t>::printKeyValuePair(OpenMap::KeyValuePair kvp) {
    std::cout << "<" << kvp.key_ << ", " << kvp.value_ << "> ";
}

template<>
void OpenMap<uint64_t, Product*>::printKeyValuePair(OpenMap::KeyValuePair kvp) {
    std::cout << "<" << kvp.key_ << ", " << kvp.value_->id << "> ";
}

template<>
void OpenMap<uint64_t, uint64_t>::printKeyValuePair(OpenMap::KeyValuePair kvp) {
    std::cout << "<" << kvp.key_ << ", " << kvp.value_ << "> ";
}
