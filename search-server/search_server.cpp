#include "search_server.h"

using namespace std;

SearchServer::SearchServer(const std::string& stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))  // Invoke delegating constructor
                                                        // from string container
{
}
SearchServer::SearchServer(const string_view stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))  // Invoke delegating constructor
                                                        // from string container
{
}


void SearchServer::AddDocument(int document_id, const string_view document, DocumentStatus status, const vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw std::invalid_argument("Invalid document_id"s);
    }
    const auto words = SplitIntoWordsNoStop(document);

    const double inv_word_count = 1.0 / words.size();
    for (const auto word : words) {
        word_to_document_freqs_[string(word)][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][string(word)] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    document_ids_.insert(document_id);
}

vector<Document> SearchServer::FindTopDocuments(const string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
    });
}

vector<Document> SearchServer::FindTopDocuments(const string_view raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

//sequenced policy
vector<Document> SearchServer::FindTopDocuments(const std::execution::sequenced_policy&, const string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
    });
}

vector<Document> SearchServer::FindTopDocuments(const std::execution::sequenced_policy&, const string_view raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

//paraller policy
vector<Document> SearchServer::FindTopDocuments(const std::execution::parallel_policy&, const string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(std::execution::par, raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
    });
}

vector<Document> SearchServer::FindTopDocuments(const std::execution::parallel_policy&, const string_view raw_query) const {
    return FindTopDocuments(std::execution::par, raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

//int SearchServer::GetDocumentId(int index) const {
//    return document_ids_.at(index);
//}

tuple<vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::string_view raw_query, int document_id) const {
    // const auto query = ParseQuery(std::execution::seq, raw_query);

    // vector<string_view> matched_words;
    // for (const auto word : query.plus_words) {
    //     if (word_to_document_freqs_.count(string(word)) == 0) {
    //         continue;
    //     }
    //     if (word_to_document_freqs_.at(string(word)).count(document_id)) {
    //         matched_words.push_back(word);
    //     }
    // }
    // for (const auto word : query.minus_words) {
    //     if (word_to_document_freqs_.count(string(word)) == 0) {
    //         continue;
    //     }
    //     if (word_to_document_freqs_.at(string(word)).count(document_id)) {
    //         matched_words.clear();
    //         break;
    //     }
    // }
    // return {matched_words, documents_.at(document_id).status};
    if(document_ids_.count(document_id) == 0) {
        throw std::out_of_range("document_id does not exist!");
    }
    const auto query = ParseQuery(std::execution::seq, raw_query);
    for (int i = 0; i < 10000; ++i) {
        string("ahalay-mahalay");
    }
    vector<string_view> matched_words(query.plus_words.size());
    if (document_ids_.count(document_id) == 0) {return { {}, {} };}

    // approach with word_to_document_freqs_
    /*
    const auto word_checker =
        [this, document_id](const string& word) {
        const auto it = word_to_document_freqs_.find(word);
        return it != word_to_document_freqs_.end() && it->second.count(document_id);
    };
    */

   //approach with document_words
   const auto& m = GetWordFrequencies(document_id);
   auto word_checker = [&m](const auto word){
       return m.count(string(word));
   };

    bool is_minus_word = any_of(std::execution::seq, query.minus_words.begin(), query.minus_words.end(), word_checker);
    if (is_minus_word) { return 
        {vector<string_view>{}, documents_.at(document_id).status};
    }
    copy_if(std::execution::seq, query.plus_words.begin(), query.plus_words.end(), matched_words.begin(), word_checker);
    sort(std::execution::seq, matched_words.begin(),matched_words.end());
    auto it_last = unique(std::execution::seq, matched_words.begin(), matched_words.end());
    matched_words.erase(it_last, matched_words.end());
    it_last = remove(std::execution::seq, matched_words.begin(), matched_words.end(), "");
    matched_words.erase(it_last, matched_words.end());
    
    
    return { matched_words, documents_.at(document_id).status };
    
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::sequenced_policy& policy, const std::string_view raw_query, int document_id) const {
    return MatchDocument(raw_query, document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::parallel_policy&, const string_view raw_query, int document_id) const {
    
    if(document_ids_.count(document_id) == 0) {
        throw std::out_of_range("document_id does not exist!");
    }
    const auto query = ParseQuery(std::execution::par, raw_query);

    vector<string_view> matched_words(query.plus_words.size());
    //if (document_ids_.count(document_id) == 0) {return { {}, {} };}

    // approach with word_to_document_freqs_
    /*
    const auto word_checker =
        [this, document_id](const string& word) {
        const auto it = word_to_document_freqs_.find(word);
        return it != word_to_document_freqs_.end() && it->second.count(document_id);
    };
    */

   //approach with document_words
   const auto& m = GetWordFrequencies(document_id);
   auto word_checker = [&m](const auto word){
       return m.count(string(word));
   };

    bool is_minus_word = any_of(std::execution::seq, query.minus_words.begin(), query.minus_words.end(), word_checker);
    if (is_minus_word) { return 
        {vector<string_view>{}, documents_.at(document_id).status};
    }
    copy_if(std::execution::par, query.plus_words.begin(), query.plus_words.end(), matched_words.begin(), word_checker);
    sort(matched_words.begin(),matched_words.end());
    auto it_last = unique(matched_words.begin(), matched_words.end());
    matched_words.erase(it_last, matched_words.end());
    it_last = remove(matched_words.begin(), matched_words.end(), "");
    matched_words.erase(it_last, matched_words.end());
    
    
    return { matched_words, documents_.at(document_id).status };
}

bool SearchServer::IsStopWord(const std::string_view word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const std::string_view word) {
    // A valid word must not contain special characters
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

vector<string_view> SearchServer::SplitIntoWordsNoStop(const string_view text) const {
    vector<string_view> words;
    for (const auto word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw invalid_argument("Word "s + string(word) + " is invalid"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(const std::string_view text) const {
    if (text.empty()) {
        throw invalid_argument("Query word is empty"s);
    }
    auto word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw invalid_argument("Query word "s + string(text) + " is invalid");
    }

    return {word, is_minus, IsStopWord(word)};
}

SearchServer::Query SearchServer::ParseQuery(const std::execution::parallel_policy&, const string_view text) const {
    Query result;
    for (const auto word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data); // ?
            } else {
                result.plus_words.push_back(query_word.data); // ?
            }
        }
    }

    return result;
}

SearchServer::Query SearchServer::ParseQuery(const std::execution::sequenced_policy&, const string_view text) const {
    Query result;
    for (const auto word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data); // ?
            } else {
                result.plus_words.push_back(query_word.data); // ?
            }
        }
    }
    sort(result.plus_words.begin(), result.plus_words.end());
    std::vector<std::string_view>::const_iterator last = unique(result.plus_words.begin(), result.plus_words.end());
    result.plus_words.erase(last, result.plus_words.end());
    sort(result.minus_words.begin(), result.minus_words.end());
    last = unique(result.minus_words.begin(), result.minus_words.end());
    result.minus_words.erase(last, result.minus_words.end());
    return result;
}

// Existence required
double SearchServer::ComputeWordInverseDocumentFreq(const string_view word) const {
    return std::log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(string(word)).size());
}

const map<string, double>& SearchServer::GetWordFrequencies(int document_id) const {
    if(document_to_word_freqs_.find(document_id) == document_to_word_freqs_.end()) {
        static map<string, double> m;
        return m;
    } else {
        return document_to_word_freqs_.at(document_id);
    }
}

void SearchServer::RemoveDocument(int document_id) {
    if(document_ids_.find(document_id) == document_ids_.end()) {
        return;
    } else {
        document_ids_.erase(document_id);
        documents_.erase(document_id);

        const map<string, double>& m = GetWordFrequencies(document_id);
        if(!m.empty()) {
            vector<const string*> v(m.size());
            transform(std::execution::seq,
                        m.begin(), m.end(), v.begin(),
                        [](auto& p){
                            return &(p.first);
                        });
            
            for_each(std::execution::seq,
                        v.begin(), v.end(),
                        [this, document_id](const auto word_ptr){
                            word_to_document_freqs_[*word_ptr].erase(document_id);
                        });
            
            document_to_word_freqs_.erase(document_id);
        }
    }
}


void SearchServer::RemoveDocument(const std::execution::sequenced_policy&, int document_id) {
    return RemoveDocument(document_id);
}

void SearchServer::RemoveDocument(const std::execution::parallel_policy&, int document_id) {
    if(document_ids_.find(document_id) == document_ids_.end()) {
        return;
    } else {
        document_ids_.erase(document_id);
        documents_.erase(document_id);

        const map<string, double>& m = GetWordFrequencies(document_id);
        if(!m.empty()) {
            vector<const string*> v(m.size());
            transform(std::execution::par,
                        m.begin(), m.end(), v.begin(),
                        [](auto& p){
                            return &(p.first);
                        });
            
            for_each(std::execution::par,
                        v.begin(), v.end(),
                        [this, document_id](const auto word_ptr){
                            word_to_document_freqs_[*word_ptr].erase(document_id);
                        });
            
            document_to_word_freqs_.erase(document_id);
        }
    }
}