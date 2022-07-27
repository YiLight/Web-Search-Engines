#include "query.h"
#include "helpers.h"
#include <iostream>
#include <fstream>

query::query(vector<string> *terms, phmap::flat_hash_map<string, lexicon_item> *lexicon_map, vector<doc_item> *doc_map, int count) {
    this->terms = terms;
    this->doc_map = doc_map;
    this->lexicon_map = lexicon_map;
    this->count = count;
}

query::~query() {
    candidates.clear();
}

void query::con_query(const string &list_path, int total_doc, long long int total_length) {
    vector<inverted_list*> lists;

    // creat required lists
    for(auto & it: *terms) {
        if(lexicon_map->count(it) == 0) {
            // if one word is not found, we need to stop searching
            cout << "Word '" << it << "' not found!" << endl;
            for(auto& itt: lists) delete itt;
            return;
        }
        cout << "Length of inverted doc_ids for '" << it << "' is: " << lexicon_map->at(it).num << endl;
        lists.emplace_back(new inverted_list(list_path, &lexicon_map->at(it)));
    }

    int did = 0, total_freq = 0, freq;
    float score = 0;

    // loop until one of the doc_ids hits its end
    while(did < total_doc) {
        did = lists.front()->get_next(did);
        if(did < 0) break;

        bool success = true;

        // 'success' remains true only when all lists have same doc id
        for(int i=1;i<lists.size();i++) {
            int now = lists[i]->get_next(did);
            if(now < 0) return;
            if(now > did) {
                did = now;
                success = false;
                break;
            }
        }

        if(success) {
            for(auto & list : lists) {
                // find all frequencies and calculate score for one candidate
                freq = list->get_cur_freq();
                score += list->get_cur_score(total_doc, total_length, doc_map->at(did-1).size, freq);
                total_freq += freq;
            }

            candidates.emplace_back();
            candidates.back().score = score;
            candidates.back().doc_id = did-1;
            candidates.back().freq = total_freq;

            // delete low score candidates when there are too many candidates
            if(candidates.size() >= count*10) {
                sort(candidates.begin(), candidates.end(), compare_candidate);
                candidates.resize(count);
            }

            did++;
            score = 0;
            total_freq = 0;
        }
    }

    for(auto& it: lists) delete it;
}

void query::dis_query(const string &list_path, int total_doc, long long int total_length) {
    vector<inverted_list*> lists;
    phmap::flat_hash_map<int, candidate_item> doc_candidate;

    // creat required lists
    for(auto & it: *terms) {
        if(lexicon_map->count(it) == 0) {
            // if one word is not found, we don't need to stop searching
            cout << "Word '" << it << "' not found!" << endl;
            continue;
        }
        cout << "Length of inverted doc_ids for '" << it << "' is: " << lexicon_map->at(it).num << endl;
        lists.emplace_back(new inverted_list(list_path, &lexicon_map->at(it)));
    }

    // loop until one of the doc_ids hits its end
    for(auto & list: lists) {
        int did = 0, freq;
        float score;
        while (did < total_doc) {
            did = list->get_next(did);
            if (did < 0) break;

            // find frequency and calculate score for one candidate
            freq = list->get_cur_freq();
            score = list->get_cur_score(total_doc, total_length, doc_map->at(did - 1).size, freq);

            doc_candidate[did - 1].freq += freq;
            doc_candidate[did - 1].score += score;

            did++;
        }
    }

    for(auto & it: doc_candidate) {
        candidates.emplace_back();
        candidates.back().score = it.second.score;
        candidates.back().doc_id = it.first;
        candidates.back().freq = it.second.freq;

        // delete low score candidates when there are too many candidates
        if (candidates.size() >= count * 10) {
            sort(candidates.begin(), candidates.end(), compare_candidate);
            candidates.resize(count);
        }
    }

    for(auto& it: lists) delete it;
}

void query::generate_result(const string &trec_path, int max_lines) {
    ifstream in(trec_path);
    if(!in.is_open()) throw runtime_error("fail to open .trec file");

    // sort all candidates using their scores
    sort(candidates.begin(), candidates.end(), compare_candidate);

    for(int i=0;i<count;i++) {
        if(i >= candidates.size()) {
            cout << endl << "Not enough candidates!" << endl;
            break;
        }
        cout << endl << "No. " << i+1 << endl;

        // find the corresponding document in the original dataset
        in.seekg(doc_map->at(candidates[i].doc_id).offset, ifstream::beg);

        cout << "URL: " << doc_map->at(candidates[i].doc_id).url << endl;
        cout << "Size of page: " << doc_map->at(candidates[i].doc_id).size << endl;
        cout << "Score: " << candidates[i].score << " Freq: " << candidates[i].freq << endl << endl;

        // we define a line as good snippet when it has all the requested terms
        // we define a line as regular snippet when it has at least one requested term
        vector<string> good_snippets, regular_snippets;
        int cnt = 0;
        string line;

        // loop in the text of a document
        getline(in, line);
        getline(in, line);
        getline(in, line);
        getline(in, line);
        while (getline(in, line)) {
            if (line == "</DOC>") break;

            bool all_terms_in_line = true, has_term_in_line = false;
            for (auto &it: *terms) {
                if (in_line(line, it)) {
                    has_term_in_line = true;
                } else {
                    all_terms_in_line = false;
                }
            }

            if(all_terms_in_line) good_snippets.emplace_back(line);
            else if(has_term_in_line) regular_snippets.emplace_back(line);
        }


        // we first look at good snippets, and we sort them in length
        // here we believe that shorter lines are better
        sort(good_snippets.begin(), good_snippets.end(), compare_snippet);

        for(auto &it: good_snippets) {
            if(it.size() > 120) {
                int tail = first_pos_in_line(it, terms);
                for(auto &itt: it.substr(tail-60<0? 0: tail-60, 120)) {
                    if (isprint(itt)) cout << itt;
                    else cout << " ";
                }
            } else {
                for(auto &itt: it) {
                    if (isprint(itt)) cout << itt;
                    else cout << " ";
                }
            }
            cout << endl << "--------------" << endl;
            if(++cnt >= max_lines) break;
        }

        // look at regular snippets if there is still some room left
        if(cnt < max_lines) {
            sort(regular_snippets.begin(), regular_snippets.end(), compare_snippet);

            for (auto &it: regular_snippets) {
                if (it.size() > 120) {
                    int tail = first_pos_in_line(it, terms);
                    for (auto &itt: it.substr(tail - 60 < 0 ? 0 : tail - 60, 120)) {
                        if (isprint(itt)) cout << itt;
                        else cout << " ";
                    }
                } else {
                    for (auto &itt: it) {
                        if (isprint(itt)) cout << itt;
                        else cout << " ";
                    }
                }
                cout << endl << "--------------" << endl;
                if (++cnt >= max_lines) break;
            }
        }

        good_snippets.clear();
        regular_snippets.clear();
    }

    in.close();
}
