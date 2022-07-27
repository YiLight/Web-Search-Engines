#ifndef HW3_QUERY_H
#define HW3_QUERY_H
#include "inverted_list.h"
#include "../../parallel_hashmap/phmap.h"

class query {
public:
    query(vector<string>* terms, phmap::flat_hash_map<string, lexicon_item>* lexicon_map, vector<doc_item>* doc_map, int count);
    ~query();
    void con_query(const string& list_path, int total_doc, long long total_length);
    void dis_query(const string& list_path, int total_doc, long long total_length);
    void generate_result(const string& trec_path, int max_lines);

private:
    vector<string>* terms;
    phmap::flat_hash_map<string, lexicon_item>* lexicon_map;
    vector<doc_item>* doc_map;

    // to store the query candidates
    vector<candidate_item> candidates;

    // defines how many results will be returned
    int count;
};


#endif //HW3_QUERY_H
