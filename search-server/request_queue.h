#pragma once
#include "search_server.h"
#include "document.h"
#include <vector>
#include <string>
#include <deque>
#include <stdexcept>



class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);
    // сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {

        vector<Document> result = server_.FindTopDocuments(raw_query, document_predicate);
        QueryResult query_result = {result, result.empty() ? true : false};
        if(requests_.size() < min_in_day_) {
            if(query_result.no_result) {
                ++NoResultRequestsCount_;
            }
            requests_.push_back(query_result);
        }
        else {
            if(requests_.front().no_result) {
                --NoResultRequestsCount_;
            }
            requests_.pop_front();
            if(query_result.no_result) {
                ++NoResultRequestsCount_;
            }
            requests_.push_back(query_result);
        }

        return result;
    }

    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);

    std::vector<Document> AddFindRequest(const std::string& raw_query);

    int GetNoResultRequests() const;
private:
    struct QueryResult {
        std::vector<Document> document_results_;
        bool no_result;// определите, что должно быть в структуре
    };
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    int NoResultRequestsCount_;

    const SearchServer& server_;
};