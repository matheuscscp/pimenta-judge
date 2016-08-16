#ifndef DATABASE_H
#define DATABASE_H

#include "json.hpp"

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
    Document retrieve(const std::string& key, const std::string& value);
    std::vector<Document> retrieve(const JSON& filter = JSON());
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
