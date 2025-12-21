#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <utility>

template <typename Value>
class HashTable {
public:
    explicit HashTable(size_t initial_capacity = 1 << 16)
        : size_(0),
          max_load_factor_(0.75f) {
        size_t cap = 1;
        while (cap < initial_capacity) cap <<= 1;
        buckets_.assign(cap, nullptr);
    }

    ~HashTable() { clear(); }

    HashTable(const HashTable&) = delete;
    HashTable& operator=(const HashTable&) = delete;

    HashTable(HashTable&& other) noexcept
        : buckets_(std::move(other.buckets_)),
          size_(other.size_),
          max_load_factor_(other.max_load_factor_) {
        other.size_ = 0;
        other.buckets_.assign(0, nullptr);
    }

    HashTable& operator=(HashTable&& other) noexcept {
        if (this == &other) return *this;
        clear();
        buckets_ = std::move(other.buckets_);
        size_ = other.size_;
        max_load_factor_ = other.max_load_factor_;
        other.size_ = 0;
        other.buckets_.assign(0, nullptr);
        return *this;
    }

    size_t size() const { return size_; }
    size_t capacity() const { return buckets_.size(); }
    float load_factor() const {
        return buckets_.empty() ? 0.0f : static_cast<float>(size_) / static_cast<float>(buckets_.size());
    }

    void clear() {
        for (auto* head : buckets_) {
            while (head) {
                Node* next = head->next;
                delete head;
                head = next;
            }
        }
        if (!buckets_.empty()) buckets_.assign(buckets_.size(), nullptr);
        size_ = 0;
    }

    Value* find(const std::string& key) {
        Node* n = findNode_(key);
        return n ? &n->value : nullptr;
    }

    const Value* find(const std::string& key) const {
        Node* n = findNode_(key);
        return n ? &n->value : nullptr;
    }

    bool contains(const std::string& key) const {
        return find(key) != nullptr;
    }

    Value& getOrCreate(const std::string& key) {
        maybeRehash_();
        const size_t idx = bucketIndex_(key, buckets_.size());
        Node* cur = buckets_[idx];
        while (cur) {
            if (cur->key == key) return cur->value;
            cur = cur->next;
        }
        Node* nn = new Node{key, Value{}, buckets_[idx]};
        buckets_[idx] = nn;
        ++size_;
        return nn->value;
    }

    void put(const std::string& key, Value value) {
        maybeRehash_();
        const size_t idx = bucketIndex_(key, buckets_.size());
        Node* cur = buckets_[idx];
        while (cur) {
            if (cur->key == key) {
                cur->value = std::move(value);
                return;
            }
            cur = cur->next;
        }
        Node* nn = new Node{key, std::move(value), buckets_[idx]};
        buckets_[idx] = nn;
        ++size_;
    }

    template <typename Fn>
    void forEach(Fn&& fn) const {
        for (auto* head : buckets_) {
            for (Node* n = head; n; n = n->next) {
                fn(n->key, n->value);
            }
        }
    }

private:
    struct Node {
        std::string key;
        Value value;
        Node* next;
    };

    std::vector<Node*> buckets_;
    size_t size_;
    float max_load_factor_;

    static uint64_t fnv1a_(const std::string& s) {
        uint64_t h = 1469598103934665603ull;
        for (unsigned char c : s) {
            h ^= static_cast<uint64_t>(c);
            h *= 1099511628211ull;
        }
        return h;
    }

    static size_t bucketIndex_(const std::string& key, size_t cap) {
        return static_cast<size_t>(fnv1a_(key)) & (cap - 1);
    }

    Node* findNode_(const std::string& key) const {
        if (buckets_.empty()) return nullptr;
        const size_t idx = bucketIndex_(key, buckets_.size());
        Node* cur = buckets_[idx];
        while (cur) {
            if (cur->key == key) return cur;
            cur = cur->next;
        }
        return nullptr;
    }

    void maybeRehash_() {
        if (buckets_.empty()) {
            buckets_.assign(1024, nullptr);
            return;
        }
        if (load_factor() <= max_load_factor_) return;
        rehash_(buckets_.size() * 2);
    }

    void rehash_(size_t new_cap) {
        size_t cap = 1;
        while (cap < new_cap) cap <<= 1;

        std::vector<Node*> nb(cap, nullptr);

        for (auto* head : buckets_) {
            while (head) {
                Node* next = head->next;
                const size_t idx = bucketIndex_(head->key, cap);
                head->next = nb[idx];
                nb[idx] = head;
                head = next;
            }
        }
        buckets_ = std::move(nb);
    }
};
