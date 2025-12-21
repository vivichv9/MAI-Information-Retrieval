#pragma once
#include <vector>
#include <algorithm>
#include <cstddef>

class PostingList {
public:
    void add(int doc_id) {
        if (docs_.empty() || doc_id > docs_.back()) {
            docs_.push_back(doc_id);
            return;
        }
        auto it = std::lower_bound(docs_.begin(), docs_.end(), doc_id);
        if (it == docs_.end() || *it != doc_id) {
            docs_.insert(it, doc_id);
        }
    }

    void addSortedUnique(int doc_id) {
        if (docs_.empty() || docs_.back() != doc_id) {
            docs_.push_back(doc_id);
        }
    }

    const std::vector<int>& docs() const { return docs_; }
    bool empty() const { return docs_.empty(); }
    size_t size() const { return docs_.size(); }

    // AND
    static PostingList And(const PostingList& a, const PostingList& b) {
        PostingList out;
        const auto& A = a.docs_;
        const auto& B = b.docs_;
        out.docs_.reserve(std::min(A.size(), B.size()));

        size_t i = 0, j = 0;
        while (i < A.size() && j < B.size()) {
            if (A[i] == B[j]) { out.docs_.push_back(A[i]); ++i; ++j; }
            else if (A[i] < B[j]) ++i;
            else ++j;
        }
        return out;
    }

    // OR
    static PostingList Or(const PostingList& a, const PostingList& b) {
        PostingList out;
        const auto& A = a.docs_;
        const auto& B = b.docs_;
        out.docs_.reserve(A.size() + B.size());

        size_t i = 0, j = 0;
        while (i < A.size() && j < B.size()) {
            if (A[i] == B[j]) { out.docs_.push_back(A[i]); ++i; ++j; }
            else if (A[i] < B[j]) { out.docs_.push_back(A[i]); ++i; }
            else { out.docs_.push_back(B[j]); ++j; }
        }
        while (i < A.size()) out.docs_.push_back(A[i++]);
        while (j < B.size()) out.docs_.push_back(B[j++]);
        return out;
    }

    // NOT
    static PostingList Not(const PostingList& universe, const PostingList& a) {
        PostingList out;
        const auto& U = universe.docs_;
        const auto& A = a.docs_;
        out.docs_.reserve(U.size());

        size_t i = 0, j = 0;
        while (i < U.size() && j < A.size()) {
            if (U[i] == A[j]) { ++i; ++j; }
            else if (U[i] < A[j]) { out.docs_.push_back(U[i]); ++i; }
            else { ++j; }
        }
        while (i < U.size()) out.docs_.push_back(U[i++]);
        return out;
    }

private:
    std::vector<int> docs_;
};
