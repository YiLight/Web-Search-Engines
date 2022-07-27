#ifndef PROJECT_TREC_READER_H
#define PROJECT_TREC_READER_H

#include "../../parallel_hashmap/phmap.h"
#include <vector>
#include "structs.h"

using namespace std;

class trec_reader {
public:
    trec_reader(const string& in_path, const string& out_path);
    ~trec_reader();
    void assign_id();
    void read();
    void write_posting();
    void parse_line(string& line, int id);

private:
    FILE* in;
    FILE* posting;
    FILE* doc_map;

    phmap::flat_hash_map<string, int> id_map;
    phmap::flat_hash_map<string, vector<item>> map;
};


#endif //PROJECT_TREC_READER_H
