#pragma once
#include <vector>
#include <string>
#include "document.h"

using namespace std;

void PrintMatchDocumentResult(int document_id, const std::vector<std::string>& words, DocumentStatus status);

#include "search_server.h"

void AddDocument(SearchServer& search_server, int document_id, const std::string& document, DocumentStatus status,
    const std::vector<int>& ratings);

void FindTopDocuments(const SearchServer& search_server, const std::string& raw_query);

void MatchDocuments(const SearchServer& search_server, const std::string& query);