#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <numeric>
using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
constexpr double RELEVANCE_EQUALITY_THRESHOLD = 1e-6;

string ReadLine()
{
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber()
{
    int result;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string &text)
{
    vector<string> words;
    string word;
    for (const char c : text)
    {
        if (c == ' ')
        {
            if (!word.empty())
            {
                words.push_back(word);
                word.clear();
            }
        }
        else
        {
            word += c;
        }
    }
    if (!word.empty())
    {
        words.push_back(word);
    }

    return words;
}

struct Document
{
    int id;
    double relevance;
    int rating;
};

enum class DocumentStatus
{
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer
{
public:
    void SetStopWords(const string &text)
    {
        for (const string &word : SplitIntoWords(text))
        {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string &document, DocumentStatus status, const vector<int> &ratings)
    {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string &word : words)
        {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id,
                           DocumentData{
                               ComputeAverageRating(ratings),
                               status});
    }

    vector<Document> FindTopDocuments(const string &raw_query, DocumentStatus target_status = DocumentStatus::ACTUAL) const
    {

        return FindTopDocuments(raw_query, [target_status](int document_id, DocumentStatus status, int rating)
                                { return status == target_status; });
    }

    template <typename Filter>
    vector<Document> FindTopDocuments(const string &raw_query, Filter filter) const
    {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, filter); // <--------

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document &lhs, const Document &rhs)
             {
                 if (abs(lhs.relevance - rhs.relevance) < RELEVANCE_EQUALITY_THRESHOLD)
                 {
                     return lhs.rating > rhs.rating;
                 }
                 else
                 {
                     return lhs.relevance > rhs.relevance;
                 }
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT)
        {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    int GetDocumentCount() const
    {
        return documents_.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string &raw_query, int document_id) const
    {
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string &word : query.plus_words)
        {
            if (word_to_document_freqs_.count(word) == 0)
            {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id))
            {
                matched_words.push_back(word);
            }
        }
        for (const string &word : query.minus_words)
        {
            if (word_to_document_freqs_.count(word) == 0)
            {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id))
            {
                matched_words.clear();
                break;
            }
        }
        return {matched_words, documents_.at(document_id).status};
    }

private:
    struct DocumentData
    {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

    bool IsStopWord(const string &word) const
    {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string &text) const
    {
        vector<string> words;
        for (const string &word : SplitIntoWords(text))
        {
            if (!IsStopWord(word))
            {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int> &ratings)
    {
        if (ratings.empty())
        {
            return 0;
        }
        int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord
    {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const
    {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-')
        {
            is_minus = true;
            text = text.substr(1);
        }
        return {
            text,
            is_minus,
            IsStopWord(text)};
    }

    struct Query
    {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string &text) const
    {
        Query query;
        for (const string &word : SplitIntoWords(text))
        {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop)
            {
                if (query_word.is_minus)
                {
                    query.minus_words.insert(query_word.data);
                }
                else
                {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    // Existence required
    double ComputeWordInverseDocumentFreq(const string &word) const
    {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template <typename Filter>
    vector<Document> FindAllDocuments(const Query &query, Filter filter) const
    {
        map<int, double> document_to_relevance;
        for (const string &word : query.plus_words)
        {
            if (word_to_document_freqs_.count(word) == 0)
            {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word))
            {
                {
                    if (filter(document_id, documents_.at(document_id).status, documents_.at(document_id).rating))
                        document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string &word : query.minus_words)
        {
            if (word_to_document_freqs_.count(word) == 0)
            {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word))
            {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance)
        {
            matched_documents.push_back({document_id,
                                         relevance,
                                         documents_.at(document_id).rating});
        }
        return matched_documents;
    }
};

void PrintDocument(const Document &document)
{
    cout << "{ "s
         << "document_id = "s << document.id << ", relevance = " << document.relevance << ", rating = "s << document.rating << " }"s << endl;
}

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
    const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}
#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
    const string& hint) {
    if (!value) {
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}
#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename Func>
void RunTestImpl(Func func, const string& s) {
    func();
    cerr << s << " OK" << endl;
}
#define RUN_TEST(func) RunTestImpl(func , #func)
// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestAddDocument(){
    {
        SearchServer server;
        int document_count = server.GetDocumentCount();
        ASSERT_EQUAL(document_count, 0);
        server.AddDocument(0, "cat in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
        server.AddDocument(1, "dog in the city"s, DocumentStatus::ACTUAL, { 2, 3, 4 });
        server.AddDocument(2, "cats and cat on the roof", DocumentStatus::ACTUAL, { 3, 4, 5 });
        
        const auto found_docs = server.FindTopDocuments("dog");
        ASSERT_EQUAL(found_docs.size(), 1u);
        const auto found_docs1 = server.FindTopDocuments("dog cat");
        ASSERT_EQUAL(found_docs1.size(), 3u);
        const auto found_docs2 = server.FindTopDocuments("");
        ASSERT_EQUAL(found_docs2.size(), 0u);
        const auto found_docs3 = server.FindTopDocuments("city -dog");
        ASSERT_EQUAL(found_docs3.size(), 1u);
        ASSERT(found_docs3[0].id == 0);
    }
}
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
    }
}
void TestExcludeMinusWordFromAddedDocument() {

    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server;
        server.AddDocument(0, "cat in the city"s, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(1, "dog in the city"s, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(2, "cats and cat on the roof", DocumentStatus::ACTUAL, ratings);
        server.AddDocument(3, "cat and cat on the roof", DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("cat in the"s);
        ASSERT_EQUAL(found_docs.size(), 4u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, 0);

        const auto found_docs1 = server.FindTopDocuments("cat -in -the"s);
        ASSERT_EQUAL(found_docs1.size(), 0u);

    }
    {
        SearchServer server;
        server.AddDocument(0, "cat in the city"s, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(1, "dog in the city"s, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(2, "cats and cat on the roof", DocumentStatus::ACTUAL, ratings);
        server.AddDocument(3, "cat and cat on the roof", DocumentStatus::ACTUAL, ratings);
        server.AddDocument(4, "bird and dog on roof", DocumentStatus::ACTUAL, ratings);
        server.AddDocument(5, "and cat on roof", DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("cat in the"s);
        ASSERT_EQUAL(found_docs.size(), 5u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, 0);

        const auto found_docs1 = server.FindTopDocuments("cat -in -the"s);
        ASSERT_EQUAL(found_docs1.size(), 1u);

    }

}
void TestMatchingDocuments() {
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server;
        server.AddDocument(0, "cat in the city"s, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(1, "dog in the city"s, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(2, "cats and cat on the roof", DocumentStatus::ACTUAL, ratings);
        server.AddDocument(3, "bird and cat on the roof", DocumentStatus::ACTUAL, ratings);
        server.AddDocument(4, "white fox and squrrel"s, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(5, "white cat fluffy tail"s, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(6, "brown squirrel with a fluffy tail", DocumentStatus::ACTUAL, ratings);
        server.AddDocument(7, "cat and cat on the roof", DocumentStatus::ACTUAL, ratings);

        const auto found_docs1 = server.MatchDocument("brown cat"s, 1);
        ASSERT(get<0>(found_docs1) == vector<string>{});
        ASSERT(get<1>(found_docs1) == DocumentStatus::ACTUAL);

        const auto found_docs2 = server.MatchDocument("brown cat"s, 2);
        ASSERT(get<0>(found_docs2) == vector<string>{"cat"});
        ASSERT(get<1>(found_docs2) == DocumentStatus::ACTUAL);

        const auto found_docs6 = server.MatchDocument("brown cat fluffy tail"s, 6);
        ASSERT(get<0>(found_docs6) == vector<string>({ "brown", "fluffy", "tail" }));
        ASSERT(get<1>(found_docs6) == DocumentStatus::ACTUAL);

        const auto found_docs7 = server.MatchDocument("brown -cat fluffy tail"s, 7);
        ASSERT(get<0>(found_docs7) == vector<string>({}));
        ASSERT(get<1>(found_docs7) == DocumentStatus::ACTUAL);

        const auto found_docs7_1 = server.MatchDocument("  123 "s, 7);
        ASSERT(get<0>(found_docs7_1) == vector<string>({}));
        ASSERT(get<1>(found_docs7_1) == DocumentStatus::ACTUAL);
    }
}
void TestSortingOnRelevance() {
    {
        const vector<int> ratings = { 1, 2, 3 };
        {
            SearchServer server;
            server.AddDocument(0, "cat in the city"s, DocumentStatus::ACTUAL, ratings);
            server.AddDocument(1, "dog in the city"s, DocumentStatus::ACTUAL, ratings);
            server.AddDocument(2, "cats and cat on the roof", DocumentStatus::ACTUAL, ratings);
            server.AddDocument(3, "bird and cat on the roof", DocumentStatus::ACTUAL, ratings);
            server.AddDocument(4, "white fox and squrrel"s, DocumentStatus::ACTUAL, ratings);
            server.AddDocument(5, "white cat fluffy tail"s, DocumentStatus::ACTUAL, ratings);
            server.AddDocument(6, "brown squirrel with a fluffy tail", DocumentStatus::ACTUAL, ratings);
            server.AddDocument(7, "cat and cat on the roof", DocumentStatus::ACTUAL, ratings);
            const auto found_docs = server.FindTopDocuments("white fluffy cat");
            ASSERT_EQUAL(found_docs.size(), 5u);
            ASSERT(found_docs[0].relevance >= found_docs[1].relevance &&
                found_docs[1].relevance >= found_docs[2].relevance &&
                found_docs[2].relevance >= found_docs[3].relevance &&
                found_docs[3].relevance >= found_docs[4].relevance);
        }
    }
}
void TestAverageRatingCalculation() {

    {
        SearchServer server;
        server.AddDocument(0, "cat in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
        server.AddDocument(1, "dog in the city"s, DocumentStatus::ACTUAL, { 2, 3, 4 });
        server.AddDocument(2, "cats and cat on the roof", DocumentStatus::ACTUAL, { 3, 4, 5 });
        server.AddDocument(3, "bird and cat on the roof", DocumentStatus::ACTUAL, { 4, 5, 6 });
        server.AddDocument(4, "white fox and squrrel"s, DocumentStatus::ACTUAL, { 6, -62, -4443 });
        server.AddDocument(5, "white cat fluffy tail"s, DocumentStatus::ACTUAL, { -56, 0, 0 });
        server.AddDocument(6, "brown squirrel with a fluffy tail", DocumentStatus::ACTUAL, {  });
        server.AddDocument(7, "cat and cat on the roof", DocumentStatus::ACTUAL, { 0, 0, 0 });
        const auto found_docs = server.FindTopDocuments("white fluffy cat");
        ASSERT_EQUAL(found_docs.size(), 5u);
        ASSERT(found_docs[0].rating == -18
            && found_docs[1].rating == -1499
            && found_docs[2].rating == 0
            && found_docs[3].rating == 0
            && found_docs[4].rating == 2);
    }
    {
        SearchServer server;
        server.SetStopWords("in on the a an");
        server.AddDocument(0, "cat in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
        server.AddDocument(1, "dog in the city"s, DocumentStatus::ACTUAL, { 2, 3, 4 });
        server.AddDocument(2, "cats and cat on the roof", DocumentStatus::ACTUAL, { 3, 4, 5 });
        server.AddDocument(3, "bird and cat on the roof", DocumentStatus::ACTUAL, { 4, 5, 6 });
        server.AddDocument(4, "white fox and squrrel"s, DocumentStatus::ACTUAL, { 6, -62, -4443 });
        server.AddDocument(5, "white cat fluffy tail"s, DocumentStatus::ACTUAL, { -56, 0, 0 });
        server.AddDocument(6, "brown squirrel with a fluffy tail", DocumentStatus::ACTUAL, {  });
        server.AddDocument(7, "cat and cat on the roof", DocumentStatus::ACTUAL, { 0, 0, 0 });
        const auto found_docs = server.FindTopDocuments("white fluffy cat");
        ASSERT_EQUAL(found_docs.size(), 5u);
        ASSERT(found_docs[0].rating == -18
            && found_docs[1].rating == -1499
            && found_docs[2].rating == 0
            && found_docs[3].rating == 2
            && found_docs[4].rating == 0);
    }
}
void TestPredicateFiltering() {
    {
        SearchServer server;
        server.AddDocument(0, "cat in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
        server.AddDocument(1, "dog in the city"s, DocumentStatus::BANNED, { 2, 3, 4 });
        server.AddDocument(2, "cats and cat on the roof", DocumentStatus::IRRELEVANT, { 3, 4, 5 });
        server.AddDocument(3, "bird and cat on the roof", DocumentStatus::REMOVED, { 4, 5, 6 });
        server.AddDocument(4, "white fox and squrrel"s, DocumentStatus::IRRELEVANT, { 6, -62, -4443 });
        server.AddDocument(5, "white cat fluffy tail"s, DocumentStatus::ACTUAL, { -56, 0, 0 });
        server.AddDocument(6, "brown squirrel with a fluffy tail", DocumentStatus::BANNED, {  });
        server.AddDocument(7, "cat and cat on the roof", DocumentStatus::IRRELEVANT, { 0, 0, 0 });
        server.AddDocument(8, "cat and cat on the roof", DocumentStatus::REMOVED, { -8, 4, 5 });
        server.AddDocument(9, "cat and cat on the roof", DocumentStatus::ACTUAL, { 0, 2, 3 });
        const auto found_docs = server.FindTopDocuments("white fluffy cat", [](int document_id, DocumentStatus status, int rating){
            return document_id % 2 == 0;
        });
        ASSERT_EQUAL(found_docs.size(), 5u);

        const auto found_docs1 = server.FindTopDocuments("white fluffy cat", [](int document_id, DocumentStatus status, int rating){
            return document_id % 2 == 0 && status == DocumentStatus::BANNED;
        });
        ASSERT_EQUAL(found_docs1.size(), 1u);
        ASSERT(found_docs1.front().id == 6);
    }
}
/*
Разместите код остальных тестов здесь
*/

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestAddDocument);
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestExcludeMinusWordFromAddedDocument);
    RUN_TEST(TestMatchingDocuments);
    RUN_TEST(TestSortingOnRelevance);
    RUN_TEST(TestAverageRatingCalculation);
    RUN_TEST(TestPredicateFiltering);

    // Не забудьте вызывать остальные тесты здесь
    /*
    Фильтрация результатов поиска с использованием предиката, задаваемого пользователем.
    Поиск документов, имеющих заданный статус.
    Корректное вычисление релевантности найденных документов.
    */
}

int main()
{
    TestSearchServer();
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);
    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, {9});
    cout << "ACTUAL by default:"s << endl;
    for (const Document &document : search_server.FindTopDocuments("пушистый ухоженный кот"s))
    {
        PrintDocument(document);
    }
    cout << "BANNED:"s << endl;
    for (const Document &document : search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED))
    {
        PrintDocument(document);
    }
    cout << "Even ids:"s << endl;
    for (const Document &document : search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating)
                                                                   { return document_id % 2 == 0; }))
    {
        PrintDocument(document);
    }
    return 0;
}
