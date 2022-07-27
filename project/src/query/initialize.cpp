#include <fstream>
#include <sstream>
#include <vector>
#include "../../parallel_hashmap/phmap.h"
#include "structs.h"

using namespace std;

static void load_lexicon(const string& path, phmap::flat_hash_map<string, lexicon_item>& lexicon) {
    ifstream in(path, ifstream::in);
    if(!in.is_open()) throw runtime_error("fail to open lexicon from disk");

    string buffer;
    string word;
    string values;
    while(getline(in, buffer)) {
        stringstream s(buffer);
        s >> word;

        s >> values;
        lexicon[word].start = stoll(values);
        s >> values;
        lexicon[word].mid = stoll(values);
        s >> values;
        lexicon[word].end = stoll(values);
        s >> values;
        lexicon[word].num = stoi(values);
    }

    in.close();
}

static bool compare_doc_map(const doc_item &l, const doc_item &r) {
    return l.doc_id < r.doc_id;
}

static void load_doc_map(const string& path, vector<doc_item>& doc_map, int& total_doc, long long& total_length) {
    ifstream in(path, ifstream::in);
    if(!in.is_open()) throw runtime_error("fail to open doc_id from disk");

    string buffer;
    string word;

    total_doc = 0;
    total_length = 0;

    while(getline(in, buffer)) {
        stringstream s(buffer);

        doc_map.emplace_back();
        s >> word;
        doc_map.back().doc_id = stoi(word);
        s >> word;
        doc_map.back().url = word;
        s >> word;
        doc_map.back().size = stoi(word);
        s >> word;
        doc_map.back().offset = stoll(word);

        total_doc++;
        total_length += doc_map.back().size;
    }

    in.close();
}
