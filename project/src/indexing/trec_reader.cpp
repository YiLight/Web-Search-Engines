#include <sstream>
#include "trec_reader.h"

trec_reader::trec_reader(const string &in_path, const string &out_path) {
    in = fopen(in_path.c_str(), "r");
    remove((out_path + "posting.txt").c_str());
    posting = fopen((out_path + "posting.txt").c_str(), "a");
    doc_map = fopen((out_path + "doc_map.txt").c_str(), "w");
    if(in == nullptr) throw runtime_error("fail to open file when reading, check your path");
    if(posting == nullptr) throw runtime_error("fail to open file when writing posting, check your path");
    if(doc_map == nullptr) throw runtime_error("fail to open file when writing doc map, check your path");

    map.reserve(2000000);
    id_map.reserve(3300000);
}

trec_reader::~trec_reader() {
    fclose(in);
    fclose(posting);
    fclose(doc_map);
}

void trec_reader::assign_id() {
    // buffers for input
    char tmp[1000];
    string buffer;

    string url;
    vector<string> urls;

    while(true) {
        // end of the input file
        if(fgets(tmp, 1000, in) == nullptr) break;

        buffer = tmp;
        buffer.pop_back();

        if(buffer == "<DOC>") {
            // ignore <DOCNO> and <TEXT> line, but get their length
            buffer = fgets(tmp, 1000, in);
            buffer = fgets(tmp, 1000, in);

            // get URL
            url = fgets(tmp, 1000, in);
            url.pop_back();
            urls.emplace_back(url);
        }
    }

    sort(urls.begin(), urls.end());

    int size = (int)urls.size();
    for(int i=0;i<size;i++) {
        id_map[urls[i]] = i+1;
    }

    fseeko64(in, 0, SEEK_SET);
}

void trec_reader::read() {
    assign_id();

    // buffers for input
    char tmp[1000];
    string buffer;

    // variables for doc map
    string url;
    long long offset = 0;
    int page_size = 0;

    int cnt = 1;
    while(true) {
        // end of the input file
        if(fgets(tmp, 1000, in) == nullptr) break;

        // pop \n at the end of every line after get length
        buffer = tmp;
        page_size += (int)buffer.length();
        buffer.pop_back();

        if(buffer == "<DOC>") {
            // ignore <DOCNO> and <TEXT> line, but get their length
            buffer = fgets(tmp, 1000, in);
            page_size += (int)buffer.length();
            buffer = fgets(tmp, 1000, in);
            page_size += (int)buffer.length();

            // get URL
            url = fgets(tmp, 1000, in);
            page_size += (int)url.length();
            url.pop_back();
        } else if(buffer == "</DOC>"){
            // save the doc to doc map file
            fputs((to_string(id_map[url]) + " " +
                   url + " " +
                   to_string(page_size) + " " +
                   to_string(offset) + "\n"
                  ).c_str(), doc_map);

            // save a partial posting every 100000 docs
            if(cnt%100000 == 0) {
                write_posting();
                map.clear();
            }

            // update page information for doc map doc_ids
            offset += page_size;
            page_size = 0;
            cnt++;
        } else if(buffer == "</TEXT>"){
            continue;
        } else {
            // parse a line of text
            parse_line(buffer, id_map[url]);
        }
    }

    // save remaining docs
    write_posting();
}

void trec_reader::write_posting() {
    for(auto& it : map) {
        stringstream ss;

        ss << it.first;

        for(auto& itt: it.second) {
            ss << " " << itt.id << "," << itt.freq;
        }
        ss << "\n";

        fputs(ss.str().c_str(), posting);
    }
}

void trec_reader::parse_line(string &line, int id) {
    int head = 0, tail = 0, size = (int)line.size();
    string word;

    // a quick slow pointer solution to split valid words
    while(head < size) {
        if(!isalnum(line[head])) {
            if(head > tail && head-tail < 20) {
                word = line.substr(tail, head-tail);
                if(!map[word].empty() && map[word].back().id == id) {
                    map[word].back().freq++;
                } else {
                    map[word].emplace_back();
                    map[word].back().id = id;
                    map[word].back().freq = 1;
                }
            }
            head++;
            tail = head;
        } else {
            line[head] = (char)tolower(line[head]);
            head++;
        }
    }
    if(head > tail && head-tail < 20) {
        word = line.substr(tail, head-tail);
        if(!map[word].empty() && map[word].back().id == id) {
            map[word].back().freq++;
        } else {
            map[word].emplace_back();
            map[word].back().id = id;
            map[word].back().freq = 1;
        }
    }
}
