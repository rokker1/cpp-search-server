// search_server_sprint_5_final
#include <iostream>
#include "document.h"
#include "paginator.h"
#include "read_input_functions.h"
#include "request_queue.h"
#include "search_server.h"
#include "string_processing.h"
#include "test_example_functions.h"
#include "log_duration.h"
#include "remove_duplicates.h"

using namespace std;


int main() {
    SearchServer search_server("and in at"s);
    RequestQueue request_queue(search_server);

    search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, {1, 2, 3});
    search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, {1, 2, 8});
    search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, {1, 3, 2});
    search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, {1, 1, 1});
    search_server.AddDocument(6, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, { 1, 1, 1 }); // to delete
    search_server.AddDocument(7, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, { 1, 1, 1 }); // to delete
    search_server.AddDocument(8, "big cat fancy collar "s, DocumentStatus::ACTUAL, { 1, 1, 1 }); // to delete
    // 1439 запросов с нулевым результатом
    for (int i = 0; i < 1439; ++i) {
        request_queue.AddFindRequest("empty request"s);
    }
    // все еще 1439 запросов с нулевым результатом
    request_queue.AddFindRequest("curly dog"s);
    // новые сутки, первый запрос удален, 1438 запросов с нулевым результатом
    request_queue.AddFindRequest("big collar"s);
    // первый запрос удален, 1437 запросов с нулевым результатом
    request_queue.AddFindRequest("sparrow"s);
    cout << "Total empty requests: "s << request_queue.GetNoResultRequests() << endl;
    
    {
        LOG_DURATION_STREAM("Long task", cout);
        MatchDocuments(search_server, "curly -dog"s);
    }
    {
        LOG_DURATION_STREAM("Long task", cout);
        FindTopDocuments(search_server, "curly -cat"s);
    }

    //search_server.RemoveDocument(1);
    RemoveDuplicates(search_server);
    
    return 0;
}