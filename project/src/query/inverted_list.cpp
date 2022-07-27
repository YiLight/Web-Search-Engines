#include "inverted_list.h"
#include <cmath>
#include <stdexcept>

using namespace std;

inverted_list::inverted_list(const string &path, lexicon_item *lexicon) {
    this->lexicon = lexicon;

    FILE * in =  fopen(path.c_str(), "rb");
    if(in == nullptr) throw runtime_error("fail to open file when reading, check your path");
    fseeko64(in, lexicon->start, SEEK_SET);

    metadata = fgetc(in);

    int id_size = (int)(lexicon->mid - lexicon->start - 1);
    doc_ids = new unsigned char[id_size + 1];
    fread(doc_ids, 1, id_size, in);

    int num_size = 2*(id_size/512) + 1;
    nums = new unsigned char[num_size + 1];
    fread(nums, 1, num_size, in);

    int freq_size = (int)(lexicon->end - lexicon->mid - num_size);
    frequencies = new unsigned char[freq_size + 1];
    fread(frequencies, 1, freq_size, in);

    fclose(in);

    block_pos = 0;
    buffer_pos = 0;
    cache.emplace_back(-1);
}

inverted_list::~inverted_list() {
    delete doc_ids;
    delete nums;
    delete frequencies;
}

void inverted_list::update_cache() {
    int first = 0;
    for(int i=0;i<4;i++) first += ((int)doc_ids[block_pos + i] << (i << 3));

    if(cache[0] == first) return;
    else {
        cache.clear();
        cache.emplace_back(first);
    }

    for(int i=1;i<128;i++) {
        if(block_pos + (i << 2) >= lexicon->mid - lexicon->start - 1) {
            buffer_pos = 0;
            return;
        }

        unsigned int word = 0;
        for(int j=0;j<4;j++) word += ((int)doc_ids[block_pos + (i << 2) + j] << (j << 3));

        unsigned int meta = (word>>28) & 0b1111;
        word &= 0x0FFFFFFF;
        switch (meta) {
            case 8:
                cache.emplace_back(cache.back() + word);
                break;
            case 7:
                for(int j=0;j<2;j++) {
                    cache.emplace_back(cache.back() + ((word >> (j * 14)) & 16383));
                }
                break;
            case 6:
                for(int j=0;j<3;j++) {
                    cache.emplace_back(cache.back() + ((word >> (j * 9)) & 511));
                }
                break;
            case 5:
                for(int j=0;j<4;j++) {
                    cache.emplace_back(cache.back() + ((word >> (j * 7)) & 127));
                }
                break;
            case 4:
                for(int j=0;j<5;j++) {
                    cache.emplace_back(cache.back() + ((word >> (j * 5)) & 31));
                }
                break;
            case 3:
                for(int j=0;j<7;j++) {
                    cache.emplace_back(cache.back() + ((word >> (j * 4)) & 15));
                }
                break;
            case 2:
                for(int j=0;j<9;j++) {
                    cache.emplace_back(cache.back() + ((word >> (j * 3)) & 7));
                }
                break;
            case 1:
                for(int j=0;j<14;j++) {
                    cache.emplace_back(cache.back() + ((word >> (j * 2)) & 3));
                }
                break;
            default:
                for(int j=0;j<28;j++) {
                    cache.emplace_back(cache.back() + ((word >> j) & 1));
                }
                break;
        }
    }

    block_pos += 512;
    buffer_pos = 0;

    int last = 0;
    for(int i=0;i<4;i++) last += ((int)doc_ids[block_pos + i] << (i << 3));
    cache.emplace_back(last);
}

int inverted_list::search_cache(int minimum) {
    for(int i=buffer_pos; i < cache.size(); i++) {
        if(cache[i] >= minimum) {
            buffer_pos = i;
            return cache[i];
        }
    }

    buffer_pos = 0;
    return -1;
}

int inverted_list::get_next(int minimum) {
    int cached = search_cache(minimum);
    if(cached > 0) return cached;

    block_pos += 512;
    while(block_pos < lexicon->mid - lexicon->start - 1) {
        int last = 0;
        for(int i=0;i<4;i++) last += ((int)doc_ids[block_pos + i] << (i << 3));

        if(last < minimum) block_pos += 512;
        else {
            block_pos -= 512;
            update_cache();
            return search_cache(minimum);
        }
    }

    block_pos -= 512;
    update_cache();
    return search_cache(minimum);
}

int inverted_list::get_cur_freq() {
    int freq = 0, tmp;
    unsigned char match;

    int num = 0;
    for(int i=0;i<block_pos/512 - 1;i++) {
        num += (int)nums[i<<1];
        num += (int)nums[1 + (i<<1)] << 8;
    }
    num += buffer_pos;

    // skip to next freq using input positions
    switch (metadata) {
        case 0b0:
            freq = 1;
            break;
        case 0b1:
            match = 0b10000000;
            tmp = num % 8;
            freq = ((frequencies[num/8]&(match >> tmp)) >> (7 - tmp)) + 1;
            break;
        case 0b10:
            match = 0b11000000;
            tmp = (num % 4) << 1;
            freq = ((frequencies[num/4]&(match >> tmp)) >> (6 - tmp)) + 1;
            break;
        case 0b11:
            match = 0b11110000;
            tmp = (num % 2) << 2;
            freq = ((frequencies[num/2]&(match >> tmp)) >> (4 - tmp)) + 1;
            break;
        case 0b100:
            freq = frequencies[num] + 1;
            break;
        case 0b101:
            freq += (int)frequencies[(num<<1)] << 8;
            freq += (int)frequencies[(num<<1) + 1];
            freq += 1;
            break;
        case 0b110:
            for(int i=0;i<4;i++) freq += ((int)frequencies[(num<<2) + i] << (i << 3));
            freq += 1;
            break;
        default:
            freq = 1;
            break;
    }

    return freq;
}

float inverted_list::get_cur_score(int total_doc, long long total_length, int page_size, int freq) const {
    // standard BM25 score
    float idf = std::log(((float)total_doc - (float)lexicon->num + 0.5f)/((float)lexicon->num + 0.5f) + 1);
    float right = ((float)freq*2.5f/((float)freq + 1.5f*(0.25f + 0.75f * (float)page_size / ((float)total_length / (float)total_doc))));
    return idf * right;
}
