#include "request_queue.h"
using namespace std;


RequestQueue::RequestQueue(const SearchServer& search_server) 
    : server_(search_server)
    {
        NoResultRequestsCount_ = 0;
        
    }

    vector<Document> RequestQueue::AddFindRequest(const string& raw_query, DocumentStatus status) {
        return AddFindRequest(raw_query, [status](int document_id, DocumentStatus document_status, int rating){
            return status == document_status;
        });
    }

    vector<Document> RequestQueue::AddFindRequest(const string& raw_query) {
        return AddFindRequest(raw_query, DocumentStatus::ACTUAL);
    }

    int RequestQueue::GetNoResultRequests() const {
        return NoResultRequestsCount_;
    }

