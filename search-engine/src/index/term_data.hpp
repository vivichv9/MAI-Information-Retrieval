#pragma once
#include <cstdint>
#include "../structures/posting_list.hpp"

struct TermData {
    PostingList postings;
    uint32_t total_tf = 0;
};
