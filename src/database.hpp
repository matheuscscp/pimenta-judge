#ifndef DATABASE_H
#define DATABASE_H

#include <functional>

#include "json.hpp"

#define DB(X) Database::Collection X(#X)

namespace Database {

typedef std::pair<const int,JSON> Document;
typedef std::function<Document(const Document&)> Transformation;
typedef std::function<bool(Document&)> Updater;

class Collection {
  // API
  public:
    Collection(const std::string& name);
    int create(const JSON& document);
    int create(JSON&& document);
    JSON retrieve(int docid);
    bool retrieve(int docid, JSON& document);
    Document retrieve(const std::string& key, const std::string& value);//FIXME allow multiple results (return a vector) and change key to index
    std::vector<Document> retrieve( // return Database::null() to discard a doc
      const Transformation& = [](const Document& doc) { return doc; }
    );
    std::vector<Document> retrieve_page(
      unsigned page,
      unsigned page_size,
      const Transformation& = [](const Document& doc) { return doc; }
    );
    bool update(int docid, const JSON& document);
    bool update(int docid, JSON&& document);
    bool update(const Updater&, int docid = 0);
    bool destroy(int docid);
  // implementation
  private:
    int collid;
};

void init();
void close();
inline Document null() { return Document(0,JSON::null()); }

} // namespace Database

#endif
