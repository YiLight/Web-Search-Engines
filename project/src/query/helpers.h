#pragma once

static bool in_line(std::string& line, const std::string& term) {
    int head = 0, tail = 0, size = (int)line.size();
    std::string word;

    // a quick slow pointer solution to split valid words
    while(head < size) {
        if(!isalnum(line[head])) {
            if(head > tail && head-tail < 20) {
                word = line.substr(tail, head-tail);
                if(word == term) return true;
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
        if(word == term) return true;
    }

    return false;
}

static int first_pos_in_line(std::string& line, const vector<string>* terms) {
    int head = 0, tail = 0, size = (int)line.size();
    std::string word;

    // a quick slow pointer solution to split valid words
    while(head < size) {
        if(!isalnum(line[head])) {
            if(head > tail && head-tail < 20) {
                word = line.substr(tail, head-tail);
                for(auto &term: *terms) {
                    if (word == term) {
                        return tail;
                    }
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
        for(auto &term: *terms) {
            if (word == term) {
                return tail;
            }
        }
    }

    return -1;
}

static bool compare_candidate(const candidate_item& l, const candidate_item& r) {
    return l.score > r.score;
}

static bool compare_snippet(const string& l, const string& r) {
    return l.size() < r.size();
}
