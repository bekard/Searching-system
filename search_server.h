#pragma once
#include "sinchronized.h"

#include <string_view>
#include <istream>
#include <ostream>
#include <vector>
#include <future>
#include <string>
#include <map>
using namespace std;

struct Id_count {
	Id_count() {}
	Id_count(const size_t& id_, const size_t& count_)
		:id(id_)
		, count(count_)
	{
	}

	size_t id;
	size_t count;
};

class InvertedIndex {
public:
	InvertedIndex();
	void Add(const string& document);
	const vector<pair<size_t, size_t>>& Lookup(const string_view& word) const;
	void FillIndex();

	const string& GetDocument(size_t id) const {
		return docs[id];
	}

	size_t DocsSize() const {
		return docs.size();
	}

private:
	map<string_view, map<size_t, size_t>> words_map;
	map<string_view, vector<pair<size_t, size_t>>> index;
	vector<pair<size_t, size_t>> empty;
	vector<string> docs;
};

class SearchServer {
public:
	SearchServer() = default;
	explicit SearchServer(istream& document_input);
	void UpdateDocumentBase(istream& document_input);
	void UpdateDBasync(vector<string> docs_vec);
	void AddQueriesStream(istream& query_input, ostream& search_results_output);

private:
	Synchronized<InvertedIndex> index;
	vector<future<void>> futures;
};
