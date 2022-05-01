#include "process_queries.h"
#include "search_server.h"
#include "my_tests.h"
#include <execution>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

// void PrintDocument(const Document& document) {
//     cout << "{ "s
//          << "document_id = "s << document.id << ", "s
//          << "relevance = "s << document.relevance << ", "s
//          << "rating = "s << document.rating << " }"s << endl;
// }

int main() {
    {
        SearchServer search_server("and with"s);

        int id = 0;
        for (
            const string& text : {
                "white cat and yellow hat"s,
                "curly cat curly tail"s,
                "nasty dog with big eyes"s,
                "nasty pigeon john"s,
            }
        ) {
            search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
        }


        cout << "ACTUAL by default:"s << endl;
        // последовательная версия
        for (const Document& document : search_server.FindTopDocuments("curly nasty cat fedor -pasha -sasha"s)) {
            PrintDocument(document);
        }
        cout << "BANNED:"s << endl;
        // последовательная версия
        for (const Document& document : search_server.FindTopDocuments(execution::seq, "curly nasty cat fedor -pasha -sasha", DocumentStatus::BANNED)) {
            PrintDocument(document);
        }

        cout << "Even ids:"s << endl;
        // параллельная версия
        for (const Document& document : search_server.FindTopDocuments(execution::par, "curly nasty cat fedor -pasha -sasha", [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
            PrintDocument(document);
        }
    }
    {
        mt19937 generator;

        const auto dictionary = GenerateDictionary(generator, 1000, 10);
        const auto documents = GenerateQueries(generator, dictionary, 50'000, 70);

        const string query = GenerateQuery(generator, dictionary, 500, 0.1);

        SearchServer search_server(dictionary[0]);
        for (size_t i = 0; i < documents.size(); ++i) {
            search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, {1, 2, 3});
        }

        TEST(seq);
        TEST(par);
    }
    return 0;
}