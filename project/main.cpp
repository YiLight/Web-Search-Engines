#include <iostream>
#include <sstream>
#include <ctime>
#include "src/query/initialize.cpp"
#include "src/indexing/index_builder.h"
#include "src/indexing/trec_reader.h"
#include "src/query/query.h"


int main() {
    string running_mode;

    cout << endl << "Please choose running mode, 'r' for read trec, 'b' for building inverted index, 'q' to query: ";
    getline(cin, running_mode);

    if (running_mode == "r") {
        string trec_path, out_path;

        cout << endl << "Please input path to your trec file: ";
        getline(cin, trec_path);

        cout << endl << "Please input path to your folder to output (with / at the end): ";
        getline(cin, out_path);

        trec_reader reader(trec_path, out_path);
        reader.read();

        cout << endl << "Reading finished! ";
    }
    else if (running_mode == "b") {
        string sorted_path, out_path;

        cout << endl << "Please input path to your sorted postings file: ";
        getline(cin, sorted_path);

        cout << endl << "Please input path to your folder to output (with / at the end): ";
        getline(cin, out_path);

        index_builder builder(sorted_path, out_path);
        while(builder.read_line());
        builder.write_line();

        cout << endl << "Building finished! ";
    }
    else if (running_mode == "q") {
//        string lexicon_path, doc_path, list_path, trec_path;
//
//        cout << endl << "Please input path to your lexicon map file: ";
//        getline(cin, lexicon_path);
//
//        cout << endl << "Please input path to your doc map file: ";
//        getline(cin, doc_path);
//
//        cout << endl << "Please input path to your inverted index file: ";
//        getline(cin, list_path);
//
//        cout << endl << "Please input path to your trec file: ";
//        getline(cin, trec_path);

        string lexicon_path = R"(D:\msmarco_dataset\test\lexicon_map.txt)";
        string doc_path = R"(D:\msmarco_dataset\test\doc_map.txt)";
        string list_path = R"(D:\msmarco_dataset\test\inverted_list.txt)";
        string trec_path = R"(D:\msmarco_dataset\msmarco-docs.trec)";

        phmap::flat_hash_map<string, lexicon_item> lexicon_map;
        vector<doc_item> doc_map;

        lexicon_map.reserve(24000000);
        doc_map.reserve(3300000);

        auto start = clock();

        cout << "Start to load lexicon_map map" << endl;
        load_lexicon(lexicon_path, lexicon_map);

        cout << "Start to load doc map" << endl;
        int total_doc = 0;
        long long total_length = 0;
        load_doc_map(doc_path, doc_map, total_doc, total_length);

        sort(doc_map.begin(), doc_map.end(), compare_doc_map);

        auto mid = clock();
        cout << "Loading time: " << (double)(mid-start)/CLOCKS_PER_SEC << endl;

        string buffer;
        string term;
        bool mode;
        vector<string> terms;
        while(true) {
            cout << endl << "Please choose query type, 'c' for conjunctive, 'd' for disjunctive, 'q' to quit: ";
            getline(cin, buffer);
            if (buffer == "c") mode = true;
            else if (buffer == "d") mode = false;
            else if (buffer == "q") break;
            else continue;

            cout << endl << "Please input your query, use SPACE to separate words: ";
            getline(cin, buffer);
            stringstream ss(buffer);
            while(ss >> term) {
                terms.emplace_back(term);
            }

            mid = clock();

            query q(&terms, &lexicon_map, &doc_map, 10);

            if(mode) q.con_query(list_path, total_doc, total_length);
            else q.dis_query(list_path, total_doc, total_length);

            q.generate_result(trec_path, 5);

            auto end = clock();
            cout << endl << "Searching time: " << (double)(end-mid)/CLOCKS_PER_SEC << endl;

            terms.clear();
        }
    }

    return 0;
}
