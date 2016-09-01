#ifndef DATABASE_H
#define DATABASE_H

#include <functional>

#include "json.hpp"

#define DB(X) Database::Collection X(#X)

namespace Database {

typedef std::pair<const int,JSON> Document;
typedef std::function<bool(Document&)> Updater;
inline Document null() { return Document(0,JSON::null()); }

class Collection {
  // API
  public:
    Collection(const std::string& name);
    int create(const JSON& document);
    int create(JSON&& document);
    JSON retrieve(int docid);
    bool retrieve(int docid, JSON& document);
    JSON retrieve(const JSON& filter = JSON());
    JSON retrieve_page(unsigned page, unsigned page_size);
    bool update(int docid, const JSON& document);
    bool update(int docid, JSON&& document);
    bool update(const Updater&, int docid = 0); // care for deadlocks!
    bool destroy(int docid);
  // implementation
  private:
    int collid;
};

void init(bool backup);
void close();

} // namespace Database

#endif
