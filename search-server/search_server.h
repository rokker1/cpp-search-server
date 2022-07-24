#pragma once
#include <vector>
#include <string>
#include <string_view>
#include <tuple>
#include <set>
#include <map>
#include <algorithm>
#include "string_processing.h"
#include "document.h"
#include <iostream>
#include <cmath>
#include <numeric>
#include <execution>
#include "log_duration.h"
#include "concurrent_map.h"

constexpr int MAX_RESULT_DOCUMENT_COUNT = 5;
constexpr double RELEVANCE_EQUALITY_TRESHOLD = 1e-6;

class SearchServer {
public:

    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words); //вынесено за пределы класса

    // Invoke delegating constructor from string container
    explicit SearchServer(const std::string& stop_words_text);

    // Invoke delegating constructor from string container
    explicit SearchServer(const std::string_view stop_words_text);
          
    void AddDocument(int document_id, const string_view document, DocumentStatus status, const std::vector<int>& ratings);
    
    // sequenced policy
    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentPredicate document_predicate) const;
    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentStatus status) const;
    std::vector<Document> FindTopDocuments(const std::string_view raw_query) const;
    
    // policy
    template <typename DocumentPredicate, typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, const string_view raw_query, DocumentPredicate document_predicate) const;
    
    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, const std::string_view raw_query, DocumentStatus status) const;
    
    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, const std::string_view raw_query) const;

    int GetDocumentCount() const;

    //int GetDocumentId(int index) const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::string_view raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::sequenced_policy& policy, const std::string_view raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::parallel_policy& policy, const std::string_view raw_query, int document_id) const;

    std::set<int>::const_iterator begin() const {
        return document_ids_.begin();
    }

    std::set<int>::const_iterator end() const {
        return document_ids_.end();
    }

    std::set<int>::iterator begin() {
        return document_ids_.begin();
    }

    std::set<int>::iterator end() {
        return document_ids_.end();
    }

    const map<string, double>& GetWordFrequencies(int document_id) const;

    void RemoveDocument(int document_id);
    void RemoveDocument(const std::execution::sequenced_policy&, int document_id);
    void RemoveDocument(const std::execution::parallel_policy&, int document_id);
private:

    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    const std::set<std::string, std::less<>> stop_words_;
    std::map<std::string, std::map<int, double>> word_to_document_freqs_;
    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;
    std::map<int, map<std::string, double>> document_to_word_freqs_;

    bool IsStopWord(const std::string_view word) const;

    static bool IsValidWord(const std::string_view word);

    std::vector<std::string_view> SplitIntoWordsNoStop(const string_view text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(const std::string_view text) const;

    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    Query ParseQuery(const std::execution::parallel_policy&, const string_view text) const;
    Query ParseQuery(const std::execution::sequenced_policy&, const string_view text) const;

    // Existence required
    double ComputeWordInverseDocumentFreq(const std::string_view word) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const std::execution::sequenced_policy&, const Query& query, DocumentPredicate document_predicate) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const std::execution::parallel_policy&, const Query& query, DocumentPredicate document_predicate) const;
};

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words))  // Extract non-empty stop words
{
    if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
        throw invalid_argument("Some of stop words are invalid"s);
    }
}

//simple - predicate -> template - predicate
template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const string_view raw_query, DocumentPredicate document_predicate) const {
   return FindTopDocuments(std::execution::seq, raw_query, document_predicate);
}

//Примеачание
/*
    Число перегрузок сокращено до 6,
    Тестирующая система не принимает решение.
    Исключение аргумента policy из сигнатуры функции и 9 перегрузок позволяют пройти тесты.
*/

//The Main Common Template Version (Policy Predicate)
template <typename DocumentPredicate, typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, const string_view raw_query, DocumentPredicate document_predicate) const {
    Query query;
    std::vector<Document> matched_documents;
    {
        //LOG_DURATION("Parallel FindTopDocuments. ParseQuery");
        query = ParseQuery(std::execution::seq, raw_query);
    }
    {
        //LOG_DURATION("Parallel FindTopDocuments. FindAllDocuments");
        matched_documents = std::move(FindAllDocuments(policy, query, document_predicate));
    }
    {
        //LOG_DURATION("Parallel FindTopDocuments. Sort");
        sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
            if (abs(lhs.relevance - rhs.relevance) < RELEVANCE_EQUALITY_TRESHOLD) {
                return lhs.rating > rhs.rating;
            } else {
                return lhs.relevance > rhs.relevance;
            }
        });
    }
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
}

//policy - status -> policy - predicate
template <typename ExecutionPolicy>
vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, const string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(policy, raw_query, [status](int, DocumentStatus document_status, int) {
        return document_status == status;
    });
}

//policy - raw_query -> policy - status
template <typename ExecutionPolicy>
vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, const string_view raw_query) const {
    return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const std::execution::sequenced_policy&, const Query& query, DocumentPredicate document_predicate) const {
    //cout << "IN: debug FindAllDocuments Seq" << endl;
    map<int, double> document_to_relevance;
    

    for (const auto word : query.plus_words) {
        if (word_to_document_freqs_.count(string(word)) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(string(word))) {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }

    for (const auto word : query.minus_words) {
        if (word_to_document_freqs_.count(string(word)) == 0) {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(string(word))) {
            document_to_relevance.erase(document_id);
        }
    }

    vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back({document_id, relevance, documents_.at(document_id).rating});
    }
    return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const std::execution::parallel_policy&, const Query& query, DocumentPredicate document_predicate) const {
    ConcurrentMap<int, double> document_to_relevance_cm(8);
    {
        //LOG_DURATION("Inside FindAllDocuments: plus_words cycle:");
        for_each(std::execution::par,
            query.plus_words.begin(), query.plus_words.end(),
            [&document_to_relevance_cm, this, &document_predicate](string_view word){
                if (word_to_document_freqs_.count(string(word)) == 0) {
                    return;
                }
                const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
                
                const std::map<int, double>& documents_with_word = word_to_document_freqs_.at(string(word));
                for_each(std::execution::par, documents_with_word.begin(), documents_with_word.end(), 
                [&document_to_relevance_cm, this, &document_predicate, inverse_document_freq](const auto id_tf){
                    const auto& document_data = documents_.at(id_tf.first);
                    if (document_predicate(id_tf.first, document_data.status, document_data.rating)) {
                        document_to_relevance_cm[id_tf.first].ref_to_value += id_tf.second * inverse_document_freq;
                    }
                });
            });
    }

    for (const auto word : query.minus_words) {
        if (word_to_document_freqs_.count(string(word)) == 0) {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(string(word))) {
            document_to_relevance_cm.erase(document_id);
        }
    }
    vector<Document> matched_documents;
    {
        //LOG_DURATION("Inside FindAllDocuments: vector<Documents> building:");
        
        for (const auto [document_id, relevance] : document_to_relevance_cm.BuildOrdinaryMap()) {
            matched_documents.push_back({document_id, relevance, documents_.at(document_id).rating});
        }
    }
    return matched_documents;
}

