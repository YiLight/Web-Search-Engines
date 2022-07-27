#pragma once
#include <string>

using namespace std;

struct lexicon_item{
    long long start{};
    long long mid{};
    long long  end{};
    int num{};
};

struct doc_item{
    int doc_id;
    string url;
    int size{};
    long long offset{};
};

struct candidate_item{
    float score;
    int doc_id;
    int freq;
};
