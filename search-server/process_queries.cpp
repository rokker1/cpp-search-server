#include "process_queries.h"

std::vector<std::vector<Document>> ProcessQueries (
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {

        std::vector<std::vector<Document>> result(queries.size());

        std::transform(std::execution::par, queries.begin(), queries.end(), result.begin(), [&search_server](const std::string& query){
            return search_server.FindTopDocuments(query);
            
        });

        return result;
}


std::list<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {
        return transform_reduce(std::execution::par,
                                queries.begin(), queries.end(),
                                std::list<Document>{},
                                [](const std::list<Document>& acum, const std::list<Document>& out){
                                    std::list<Document> l{acum.begin(), acum.end()};
                                    l.insert(l.end(), out.begin(), out.end());
                                    return l;
                                },
                                [&search_server](const std::string& query){
                                    std::vector<Document> out_vector(search_server.FindTopDocuments(query));
                                    return std::list<Document>{out_vector.begin(), out_vector.end()};
                                });
    }