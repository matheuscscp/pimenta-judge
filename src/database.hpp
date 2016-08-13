#ifndef DATABASE_H
#define DATABASE_H

#include "json.hpp"

namespace Database {

void init();
void close();

class Collection {
  // API
  public:
    Collection(const std::string& name);
    int create(const JSON& document);
    int create(JSON&& document);
    JSON retrieve(int docid);
    bool retrieve(int docid, JSON& document);
    bool update(int docid, const JSON& document);
    bool update(int docid, JSON&& document);
    bool destroy(int docid);
  // implementation
  private:
    int collid;
};

} // namespace Database

#endif
