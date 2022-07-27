#ifndef HW3_INVERTED_LIST_H
#define HW3_INVERTED_LIST_H
#include "structs.h"
#include <string>
#include <vector>

class inverted_list {
public:
    inverted_list(const string& path, lexicon_item* lexicon);
    ~inverted_list();
    void update_cache();
    int search_cache(int minimum);
    int get_next(int minimum);
    int get_cur_freq();
    float get_cur_score(int total_doc, long long total_length, int page_size, int freq) const;

private:
    unsigned char metadata;
    unsigned char * doc_ids;
    unsigned char * nums;
    unsigned char * frequencies;

    vector<int> cache;
    int buffer_pos;
    int block_pos;

    lexicon_item* lexicon;
};


#endif //HW3_INVERTED_LIST_H
