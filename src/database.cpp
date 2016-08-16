#include <map>

#include <unistd.h>

#include "database.hpp"

#include "cmap.hpp"

#define MAX_COLLECTIONS 100
#define WRITE_INTERVAL  30

using namespace std;

static bool match(const JSON& json, const JSON& filter) {
  return true;//TODO
}

struct Coll {
  pthread_mutex_t mutex;
  string name;
  cmap<int,JSON> documents;
  Coll() : mutex(PTHREAD_MUTEX_INITIALIZER) {}
  void read() { // this function is called only once, by a locked piece of code
    JSON tmp;
    if (!tmp.read_file("database/"+name+".json")) return;
    for (auto& doc : tmp.arr()) documents[doc("_id")] = doc("document");
  }
  void write() {
    JSON tmp(vector<JSON>{});
    pthread_mutex_lock(&mutex);
    for (auto& kv : documents) tmp.emplace_back(move(map<string,JSON>{
      {"_id"      , kv.first},
      {"document" , kv.second}
    }));
    pthread_mutex_unlock(&mutex);
    tmp.write_file("database/"+name+".json");
  }
  int create(JSON&& doc) {
    int id = 1;
    pthread_mutex_lock(&mutex);
    if (documents.size() > 0) id += documents.max_key();
    documents[id] = move(doc);
    pthread_mutex_unlock(&mutex);
    return id;
  }
  bool retrieve(int id, JSON& doc) {
    pthread_mutex_lock(&mutex);
    auto it = documents.find(id);
    if (it == documents.end()) {
      pthread_mutex_unlock(&mutex);
      return false;
    }
    doc = it->second;
    pthread_mutex_unlock(&mutex);
    return true;
  }
  vector<Database::Document> retrieve(const JSON& filter) {
    if (!filter.isobj()) return vector<Database::Document>();
    if (filter.size() == 0) return retrieve_page(0,documents.size());
    JSON in(move(filter("in")));
    vector<Database::Document> ans;
    pthread_mutex_lock(&mutex);
    if (!in || !in.isarr()) {
      for (auto& kv : documents) if (match(kv.second,filter)) ans.push_back(kv);
    }
    else {
      for (auto& o : in.arr()) {
        int id;
        if (!o.read(id) || id <= 0) continue;
        auto it = documents.find(id);
        if (it == documents.end()) continue;
        if (match(it->second,filter)) ans.push_back(*it);
      }
    }
    pthread_mutex_unlock(&mutex);
    return ans;
  }
  vector<Database::Document> retrieve_page(unsigned p, unsigned ps) {
    vector<Database::Document> ans;
    if (ps) {
      pthread_mutex_lock(&mutex);
      auto it = documents.at(p*ps);
      for (int i = 0; i < ps && it != documents.end(); i++, it++) {
        ans.push_back(*it);
      }
      pthread_mutex_unlock(&mutex);
    }
    return ans;
  }
  bool update(int id, JSON&& doc) {
    pthread_mutex_lock(&mutex);
    auto it = documents.find(id);
    if (it == documents.end()) {
      pthread_mutex_unlock(&mutex);
      return false;
    }
    it->second = move(doc);
    pthread_mutex_unlock(&mutex);
    return true;
  }
  bool destroy(int id) {
    pthread_mutex_lock(&mutex);
    auto it = documents.find(id);
    if (it == documents.end()) {
      pthread_mutex_unlock(&mutex);
      return false;
    }
    documents.erase(it);
    pthread_mutex_unlock(&mutex);
    return true;
  }
};
static Coll collection[MAX_COLLECTIONS];
static int ncolls = 0;
static pthread_mutex_t colls_mutex = PTHREAD_MUTEX_INITIALIZER;
static int get(const string& name) {
  static map<string,int> colls;
  int i;
  pthread_mutex_lock(&colls_mutex);
  auto it = colls.find(name);
  if (it != colls.end()) {
    i = it->second;
    pthread_mutex_unlock(&colls_mutex);
    return i;
  }
  i = ncolls++;
  colls[name] = i;
  collection[i].name = name;
  collection[i].read();
  pthread_mutex_unlock(&colls_mutex);
  return i;
}

// thread
static bool quit = false;
static pthread_t db;
static void update() {
  int nc;
  pthread_mutex_lock(&colls_mutex);
  nc = ncolls;
  pthread_mutex_unlock(&colls_mutex);
  for (int i = 0; i < nc; i++) collection[i].write();
}
static void* thread(void*) {
  static time_t upd = 0;
  while (!quit) {
    if (upd <= time(nullptr)) {
      update();
      upd = time(nullptr)+WRITE_INTERVAL;
    }
    usleep(100000);
  }
}

namespace Database {

void init() {
  system("mkdir -p database");
  pthread_create(&db,nullptr,thread,nullptr);
}

void close() {
  quit = true;
  pthread_join(db,nullptr);
  update();
}

Collection::Collection(const string& name) : collid(get(name)) {
  
}

int Collection::create(const JSON& document) {
  JSON tmp(document);
  return collection[collid].create(move(tmp));
}

int Collection::create(JSON&& document) {
  return collection[collid].create(move(document));
}

JSON Collection::retrieve(int docid) {
  JSON ans;
  collection[collid].retrieve(docid,ans);
  return ans;
}

bool Collection::retrieve(int docid, JSON& document) {
  return collection[collid].retrieve(docid,document);
}

vector<Document> Collection::retrieve(const JSON& filter) {
  return collection[collid].retrieve(filter);
}

vector<Document> Collection::retrieve_page(unsigned page, unsigned page_size) {
  return collection[collid].retrieve_page(page,page_size);
}

bool Collection::update(int docid, const JSON& document) {
  JSON tmp(document);
  return collection[collid].update(docid,move(tmp));
}

bool Collection::update(int docid, JSON&& document) {
  return collection[collid].update(docid,move(document));
}

bool Collection::destroy(int docid) {
  return collection[collid].destroy(docid);
}

} // namespace Database
