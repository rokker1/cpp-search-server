#pragma once
#include "search_server.h"
#include "process_queries.h"
#include "document.h"
#include <execution>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "log_duration.h"

using namespace std;

string GenerateWord(mt19937& generator, int max_length) {
    const int length = uniform_int_distribution(1, max_length)(generator);
    string word;
    word.reserve(length);
    for (int i = 0; i < length; ++i) {
        word.push_back(uniform_int_distribution('a', 'z')(generator));
    }
    return word;
}

vector<string> GenerateDictionary(mt19937& generator, int word_count, int max_length) {
    vector<string> words;
    words.reserve(word_count);
    for (int i = 0; i < word_count; ++i) {
        words.push_back(GenerateWord(generator, max_length));
    }
    sort(words.begin(), words.end());
    words.erase(unique(words.begin(), words.end()), words.end());
    return words;
}

string GenerateQuery(mt19937& generator, const vector<string>& dictionary, int word_count, double minus_prob = 0) {
    string query;
    for (int i = 0; i < word_count; ++i) {
        if (!query.empty()) {
            query.push_back(' ');
        }
        if (uniform_real_distribution<>(0, 1)(generator) < minus_prob) {
            query.push_back('-');
        }
        query += dictionary[uniform_int_distribution<int>(0, dictionary.size() - 1)(generator)];
    }
    return query;
}

vector<string> GenerateQueries(mt19937& generator, const vector<string>& dictionary, int query_count, int max_word_count) {
    vector<string> queries;
    queries.reserve(query_count);
    for (int i = 0; i < query_count; ++i) {
        queries.push_back(GenerateQuery(generator, dictionary, max_word_count));
    }
    return queries;
}

template <typename ExecutionPolicy>
void Test(string_view mark, SearchServer search_server, const string& query, ExecutionPolicy&& policy) {
    LOG_DURATION(mark);
    std::vector<Document> res = search_server.FindTopDocuments(policy, query);
}

#define TEST(policy) Test(#policy, search_server, query, execution::policy)