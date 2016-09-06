#include <map>

#include <unistd.h>

#include "database.hpp"

#include "cmap.hpp"

#define MAX_COLLECTIONS 100
#define WRITE_INTERVAL  30

using namespace std;

static int backup = -1;

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
      doc.setnull();
      pthread_mutex_unlock(&mutex);
      return false;
    }
    doc = it->second;
    pthread_mutex_unlock(&mutex);
    return true;
  }
  JSON retrieve(const JSON& filter) {
    JSON ans(vector<JSON>{});
    pthread_mutex_lock(&mutex);
    for (auto& kv : documents) if (filter.issubobj(kv.second)) {
      JSON tmp = kv.second;
      tmp["id"] = kv.first;
      ans.push_back(move(tmp));
    }
    pthread_mutex_unlock(&mutex);
    return ans;
  }
  JSON retrieve_page(unsigned p, unsigned ps) {
    JSON ans(vector<JSON>{});
    pthread_mutex_lock(&mutex);
    if (!ps) p = 0, ps = documents.size();
    auto it = documents.at(p*ps);
    for (int i = 0; i < ps && it != documents.end(); i++, it++) {
      JSON tmp = it->second;
      tmp["id"] = it->first;
      ans.push_back(move(tmp));
    }
    pthread_mutex_unlock(&mutex);
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
  bool update(const Database::Updater& upd, int id) {
    bool ans = false;
    pthread_mutex_lock(&mutex);
    auto it = documents.find(id);
    if (it != documents.end()) {
      ans = upd(*it);
      pthread_mutex_unlock(&mutex);
      return ans;
    }
    for (auto& kv : documents) ans = upd(kv) || ans;
    pthread_mutex_unlock(&mutex);
    return ans;
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
static void do_backup() {
  if (backup < 0) return;
  stringstream ss;
  ss << "backup" << backup;
  system(("mkdir -p database/"+ss.str()).c_str());
  system(("cp database/*.json database/"+ss.str()).c_str());
  backup = 1-backup;
}
static void update() {
  int nc;
  pthread_mutex_lock(&colls_mutex);
  nc = ncolls;
  pthread_mutex_unlock(&colls_mutex);
  for (int i = 0; i < nc; i++) collection[i].write();
  do_backup();
  sync();
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

JSON Collection::retrieve(const JSON& filter) {
  return collection[collid].retrieve(filter);
}

JSON Collection::retrieve_page(unsigned page,unsigned page_size) {
  return collection[collid].retrieve_page(page,page_size);
}

bool Collection::update(int docid, const JSON& document) {
  JSON tmp(document);
  return collection[collid].update(docid,move(tmp));
}

bool Collection::update(int docid, JSON&& document) {
  return collection[collid].update(docid,move(document));
}

bool Collection::update(const Updater& upd, int docid) {
  return collection[collid].update(upd,docid);
}

bool Collection::destroy(int docid) {
  return collection[collid].destroy(docid);
}

void init(bool backup) {
  if (backup) ::backup = 0;
  system("mkdir -p database");
  pthread_create(&db,nullptr,thread,nullptr);
}

void close() {
  quit = true;
  pthread_join(db,nullptr);
  update();
}

} // namespace Database
