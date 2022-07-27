#include "index_builder.h"
#include <cstring>
#include <stdexcept>
#include <cmath>
#include <algorithm>

index_builder::index_builder(const string &in_path, const string &out_path) {
    in = fopen(in_path.c_str(), "r");
    list = fopen((out_path + "inverted_list.txt").c_str(), "wb");
    lexicon = fopen((out_path + "lexicon_map.txt").c_str(), "w");
    if(in == nullptr) throw runtime_error("fail to open file when reading, check your path");
    if(list == nullptr) throw runtime_error("fail to open file when writing inverted doc_ids, check your path");
    if(lexicon == nullptr) throw runtime_error("fail to open file when writing lexicon map, check your path");

    buffer = new char[30000000];

    offset = 0;
    metadata = 0b0;
}

index_builder::~index_builder() {
    fclose(in);
    fclose(list);
    fclose(lexicon);
    delete buffer;
}

bool index_builder::read_line() {
    if(fgets(buffer, 30000000, in) == nullptr) return false;

    char * new_term = strtok(buffer, ", ");
    if(new_term != old_term) {
        write_line();
        old_term = new_term;
        id_freq.clear();
        metadata = 0b0;
    }

    int id, freq, meta;
    char * token = strtok(nullptr, ", ");
    while(token != nullptr) {
        id_freq.emplace_back();

        id = atoi(token);
        id_freq.back().id = id;

        token = strtok(nullptr, ", ");
        freq = atoi(token);
        id_freq.back().freq = freq - 1;

        meta = (int)ceil(log2(log2(freq))) + 1;
        metadata = metadata < meta ? meta : metadata;

        token = strtok(nullptr, ", ");
    }

    return true;
}

static int find_max(const vector<int>& diffs, int num) {
    int max = 0, cnt = num > diffs.size() ? (int)diffs.size() : num;

    for(int i=0;i<cnt;i++) {
        max = max < diffs[i] ? diffs[i] : max;
    }

    return max;
}

static bool compare_item(const item &l, const item &r) {
    return l.id < r.id;
}

void index_builder::write_line() {
    if(old_term.empty()) return;

    sort(id_freq.begin(), id_freq.end(), compare_item);

    // save term and start offset
    fputs(old_term.c_str(), lexicon);
    fputs((" " + to_string(offset)).c_str(), lexicon);

    // save metadata to inverted doc_ids
    fputc(metadata, list);
    offset += sizeof(char);

    // save doc ids to inverted doc_ids in simple9 coding
    int size = (int)id_freq.size(), head = 0;
    vector<int16_t> nums;
    while(head < size) {
        fwrite(&id_freq[head].id, sizeof(int), 1, list);
        offset += sizeof(int);

        int cur = head + 1;
        int16_t num = 1;
        for (int i = 1; i < 128; i++) {
            if(cur >= size) break;

            vector<int> diffs;
            for(;cur<size;cur++) {
                if(diffs.size() < 28) diffs.emplace_back(id_freq[cur].id - id_freq[cur-1].id);
                else break;
            }
            while(diffs.size() < 28) {
                diffs.emplace_back(0);
                cur++;
            }

            unsigned int word = 0;
            if(find_max(diffs, 28) <= 1) {
                for(int j=0;j<28;j++) {
                    word |= diffs[j] << j;
                }
                num += 28;
            } else if(find_max(diffs, 14) <= 3) {
                for(int j=0;j<14;j++) {
                    word |= diffs[j] << (j<<1);
                }
                word |= 1 << 28;
                cur -= 14;
                num += 14;
            } else if(find_max(diffs, 9) <= 7) {
                for(int j=0;j<9;j++) {
                    word |= diffs[j] << (j * 3);
                }
                word |= 2 << 28;
                cur -= 19;
                num += 9;
            } else if(find_max(diffs, 7) <= 15) {
                for(int j=0;j<7;j++) {
                    word |= diffs[j] << (j<<2);
                }
                word |= 3 << 28;
                cur -= 21;
                num += 7;
            } else if(find_max(diffs, 5) <= 31) {
                for(int j=0;j<5;j++) {
                    word |= diffs[j] << (j * 5);
                }
                word |= 4 << 28;
                cur -= 23;
                num += 5;
            } else if(find_max(diffs, 4) <= 127) {
                for(int j=0;j<4;j++) {
                    word |= diffs[j] << (j * 7);
                }
                word |= 5 << 28;
                cur -= 24;
                num += 4;
            } else if(find_max(diffs, 3) <= 511) {
                for(int j=0;j<3;j++) {
                    word |= diffs[j] << (j * 9);
                }
                word |= 6 << 28;
                cur -= 25;
                num += 3;
            } else if(find_max(diffs, 2) <= 16383) {
                for(int j=0;j<2;j++) {
                    word |= diffs[j] << (j * 14);
                }
                word |= 7 << 28;
                cur -= 26;
                num += 2;
            } else {
                word = diffs[0];
                word |= 8 << 28;
                cur -= 27;
                num += 1;
            }

            fwrite(&word, sizeof(int), 1, list);
            offset += sizeof(int);
        }

        // save number of doc ids in a block
        nums.emplace_back(num);

        head = cur;
    }

    // save doc ids end offset
    fputs((" " + to_string(offset)).c_str(), lexicon);

    // save num of doc id in each block to list
    for(auto & it: nums) {
        fwrite(&it, sizeof(int16_t), 1, list);
        offset += sizeof(int16_t);
    }

    // save freq to inverted doc_ids
    unsigned char freq_buffer = 0, times = 0;
    switch(metadata&0b111) {
        case 0b0:
            break;
        case 0b1:
            for(auto& it: id_freq) {
                freq_buffer = freq_buffer << 1;
                freq_buffer += it.freq;
                times++;
                if(times == 8) {
                    fputc(freq_buffer, list);
                    times = 0;
                }
            }
            if(times != 0) {
                freq_buffer = freq_buffer << (8-times);
                fputc(freq_buffer, list);
                offset = offset + sizeof(char) * (size/8 + 1);
            } else {
                offset = offset + sizeof(char) * (size/8);
            }
            break;
        case 0b10:
            for(auto& it: id_freq) {
                freq_buffer = freq_buffer << 2;
                freq_buffer += it.freq;
                times++;
                if(times == 4) {
                    fputc(freq_buffer, list);
                    times = 0;
                }
            }
            if(times != 0) {
                freq_buffer = freq_buffer << (4-times)*2;
                fputc(freq_buffer, list);
                offset = offset + sizeof(char) * (size/4 + 1);
            } else {
                offset = offset + sizeof(char) * (size/4);
            }
            break;
        case 0b11:
            for(auto& it: id_freq) {
                freq_buffer = freq_buffer << 4;
                freq_buffer += it.freq;
                times++;
                if(times == 2) {
                    fputc(freq_buffer, list);
                    times = 0;
                }
            }
            if(times != 0) {
                freq_buffer = freq_buffer << 4;
                fputc(freq_buffer, list);
                offset = offset + sizeof(char) * (size/2 + 1);
            } else {
                offset = offset + sizeof(char) * (size/2);
            }
            break;
        case 0b100:
            for(auto& it: id_freq) {
                fputc(it.freq, list);
            }
            offset = offset + sizeof(char) * size;
            break;
        case 0b101:
            for(auto& it: id_freq) {
                fwrite(&it.freq, sizeof(char), 2, list);
            }
            offset = offset + sizeof(char) * size * 2;
            break;
        case 0b110:
            for(auto& it: id_freq) {
                fwrite(&it.freq, sizeof(int), 1, list);
            }
            offset = offset + sizeof(int) * size;
            break;
        default:
            for(auto& it: id_freq) {
                fwrite(&it.freq, sizeof(int), 1, list);
            }
            offset = offset + sizeof(int) * size;
    }

    // save freq end offset
    fputs((" " + to_string(offset) + " " + to_string(size) + "\n").c_str(), lexicon);
}
