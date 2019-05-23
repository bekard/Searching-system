#include "search_server.h"
#include "iterator_range.h"

#include <algorithm>
#include <functional>
#include <iterator>
#include <sstream>
#include <iostream>

vector<string> SplitIntoDocuments(istream& is) {
	vector<string> result;
	for (string line; getline(is, line);)
		result.push_back(move(line));
	return result;
}

vector<string_view> SplitIntoWords(const string& line) {
	string_view strv = line;
	string_view delims = " ";
	vector<string_view> output;
	size_t first = 0;

	while (first < strv.size())
	{
		const auto second = strv.find_first_of(delims, first);

		if (first != second)
			output.emplace_back(strv.substr(first, second - first));

		if (second == string_view::npos)
			break;

		first = second + 1;
	}

	return output;
}

SearchServer::SearchServer(istream& document_input) {
	this->UpdateDBasync(SplitIntoDocuments(document_input));
}

void SearchServer::UpdateDocumentBase(istream& document_input) {
	futures.push_back(async(launch::async, &SearchServer::UpdateDBasync,
		this, move(SplitIntoDocuments(document_input))));
}

void SearchServer::UpdateDBasync(vector<string> docs_vec) {
	InvertedIndex new_index;

	for (auto& doc : docs_vec) {
		new_index.Add(move(doc));
	}
	new_index.FillIndex();

	auto access = index.GetAccess();
	access.ref_to_value = move(new_index);
}

void SearchServer::AddQueriesStream(
	istream& query_input, ostream& search_results_output) {

	

	for (string current_query; getline(query_input, current_query); ) {

		size_t docs_size;
		{
			auto access = index.GetAccess();
			docs_size = access.ref_to_value.DocsSize();
		}
		size_t min;
		vector<pair<size_t, size_t>> docid_count(docs_size);
		docs_size < 5 ? min = docs_size : min = 5;

		const auto words = SplitIntoWords(current_query);

		for (const auto& word : words) {
//			auto vector_of_pairs = access.ref_to_value.Lookup(word);
			auto vector_of_pairs = index.GetAccess().ref_to_value.Lookup(word);
			for (const auto& [id, count] : vector_of_pairs) {
				docid_count[id].first = id;
				docid_count[id].second += count;
			}
		}

		partial_sort(
			docid_count.begin(), docid_count.begin() + min,
			docid_count.end(),
			[](pair<size_t, size_t> lhs, pair<size_t, size_t> rhs) {
			int64_t lhs_docid = lhs.first;
			auto lhs_hit_count = lhs.second;
			int64_t rhs_docid = rhs.first;
			auto rhs_hit_count = rhs.second;
			return make_pair(lhs_hit_count, -lhs_docid) > make_pair(rhs_hit_count, -rhs_docid);
		}
		);

		search_results_output << current_query << ':';
		for (auto[docid, hitcount] : Head(docid_count, 5)) {
			if (hitcount != 0) {
				search_results_output << " {"
					<< "docid: " << docid << ", "
					<< "hitcount: " << hitcount << '}';
			}
		}
		search_results_output << endl;
	}
}

InvertedIndex::InvertedIndex() {
	docs.reserve(50'000);
}

void InvertedIndex::Add(const string& document) {
	docs.push_back(document);

	const size_t docid = docs.size() - 1;
	for (const auto& word : SplitIntoWords(docs.back())) {
		//index[word].push_back(docid);
		words_map[word][docid]++;
	}
}

void InvertedIndex::FillIndex() {
	for (auto&[word, map] : words_map) {
		index[word] = vector<pair<size_t, size_t>>(map.begin(), map.end());
	}
}

const vector<pair<size_t, size_t>>& InvertedIndex::Lookup(const string_view& word) const {
	if (auto it = index.find(word); it != index.end()) {
		return it->second;
	}
	else {
		return empty;
	}
}
