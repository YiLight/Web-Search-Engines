#ifndef PROJECT_INDEX_BUILDER_H
#define PROJECT_INDEX_BUILDER_H

#include <vector>
#include <string>
#include "structs.h"

using namespace std;

class index_builder {
public:
    index_builder(const string& in_path, const string& out_path);
    ~index_builder();
    bool read_line();
    void write_line();

private:
    FILE* in;
    FILE* list;
    FILE* lexicon;
    string old_term;
    char* buffer;

    vector<item> id_freq;

    unsigned long long offset;
    unsigned char metadata;
};


#endif //PROJECT_INDEX_BUILDER_H
