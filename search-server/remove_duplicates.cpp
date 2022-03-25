#include "remove_duplicates.h"

bool CompareMapByKeys(const map<string, double>& m1, const map<string, double>& m2) {
	for (const auto& [key, value] : m1) {
		if (!m2.count(key)) {
			return false;
		}
		else {
			continue;
		}
	}
	for (const auto& [key, value] : m2) {
		if (!m1.count(key)) {
			return false;
		}
		else {
			continue;
		}
	}
	return true;
}

void RemoveDuplicates(SearchServer& search_server)
{
	map<int, map<string, double>> id_to_word_freqs;
	//map<int, set<string>> set_id_to_words;
	for (const int document_id : search_server) {
		id_to_word_freqs.insert({ document_id, search_server.GetWordFrequencies(document_id) });
	}

	vector<int> documents_to_remove;

	for (auto i = id_to_word_freqs.begin(); i != --(id_to_word_freqs.end()); ++i) {
		for (auto j = next(i); j != id_to_word_freqs.end(); ++j) {
			//cout << (i->first) << ", " << (j->first) << endl; //debug
			if (i->second.size() == j->second.size()) {
				if (CompareMapByKeys(i->second, j->second)) {
					documents_to_remove.push_back(j->first);;
				}
			}
			else {
				continue;
			}
		}
	}
	for (const int i : documents_to_remove) {
		search_server.RemoveDocument(i);
	}
}
