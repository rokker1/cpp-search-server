#pragma once

#include <vector>
#include <algorithm>
#include <numeric>
#include <execution>
#include <string>
#include <list>
#include <utility>

#include "document.h"
#include "search_server.h"

std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries);

std::list<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries);