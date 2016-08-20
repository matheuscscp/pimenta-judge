#ifndef DATABASE_H
#define DATABASE_H

#include <functional>

#include "json.hpp"

#define DB(X) Database::Collection X(#X)

namespace Database {

void init();
void close();

typedef std::pair<int,JSON> Document;
class Collection {
  // API
  public:
    Collection(const std::string& name);
    int create(const JSON& document);
    int create(JSON&& document);
    JSON retrieve(int docid);
    bool retrieve(int docid, JSON& document);
    Document retrieve(const std::string& key, const std::string& value);//FIXME allow multiple results (return a vector)
    std::vector<Document> retrieve(
      const std::function<bool(const Document&)>& accept = [](const Document&) {
        return true;
      }
    );
    std::vector<Document> retrieve_page(unsigned page, unsigned page_size);
    bool update(int docid, const JSON& document);
    bool update(int docid, JSON&& document);
    bool destroy(int docid);
  // implementation
  private:
    int collid;
};

} // namespace Database

#endif
