#include <cmath>
#include <fstream>

#include "webserver.hpp"

#include "helper.hpp"
#include "global.hpp"
#include "httpserver.hpp"
#include "user.hpp"
#include "problem.hpp"
#include "attempt.hpp"
#include "contest.hpp"

using namespace std;

static void log(const string& msg) {
  static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_mutex_lock(&mutex);
  FILE* fp = fopen("log.txt","a");
  fprintf(fp,"%s\n",msg.c_str());
  fclose(fp);
  pthread_mutex_unlock(&mutex);
}

static bool eqip(uint32_t a, uint32_t b) {
  if ((a == 0 || a == 0x0100007f) && (b == 0 || b == 0x0100007f)) return true;
  return a == b;
}

class Session : public HTTP::Session {
  public:
    uint32_t ip;
    User::Data user;
    Session(uint32_t ip, User::Data&& user) : ip(ip), user(move(user)) {
      uint32_t oip = -1;
      destroy([&](const HTTP::Session* sess) {
        auto& ref = *(Session*)sess;
        if (user.username != ref.user.username) return false;
        oip = ref.ip;
        return true;
      });
      if (oip == -1) return;
      time_t now = time(nullptr);
      tm ti;
      char buf[26];
      strftime(buf,26,"At %H:%M:%S on %d/%m/%Y",localtime_r(&now,&ti));
      log(stringf(
        "%s, username %s: IP %s logged in while IP %s was already logged in.",
        buf,
        user.username.c_str(),
        HTTP::iptostr(ip).c_str(),
        HTTP::iptostr(oip).c_str()
      ));
    }
    HTTP::Session* clone() const { return new Session(*this); }
};

class Handler : public HTTP::Handler {
public:

Session& castsess() { return *(Session*)session(); }

void index(const vector<string>& segments = vector<string>()) {
  string fn = move(HTTP::path(segments,"www"));
  if (fn != "") file(fn);
  else not_found();
}

Handler() {

route("/",[=](const vector<string>& segments) {
  index(
    session() || segments.size() > 0 ? segments : vector<string>{"login.html"}
  );
});

// =============================================================================
// GET
// =============================================================================

route("/status",[=](const vector<string>&) {
  DB(languages);
  auto langdocs = languages.retrieve();
  JSON langs(vector<JSON>{});
  for (auto& kv : langdocs) {
    kv.second["id"] = kv.first;
    langs.push_back(move(kv.second));
  }
  json(map<string,JSON>{
    {"fullname" , castsess().user.fullname},
    {"languages", langs}
  });
},true);

route("/problems",[=](const vector<string>& args) {
  if (args.size() < 2) { json(Problem::page()); return; }
  unsigned page, page_size;
  if (!read(args[0],page) || !read(args[1],page_size)) {
    json(Problem::page());
    return;
  }
  json(Problem::page(page,page_size));
},true);

route("/logout",[=](const vector<string>&) {
  session(nullptr);
  location("/");
},true);

route("/problem",[=](const vector<string>& args) {
  int probid;
  if (!read(args[0],probid)) { not_found(); return; }
  json(Problem::get(probid));
},true,false,1);

route("/problem/statement",[=](const vector<string>& args) {
  int probid;
  if (!read(args[0],probid)) { not_found(); return; }
  string fn = Problem::statement(probid);
  if (fn == "") { not_found(); return; }
  file(fn);
},true,false,1);

route("/source",[=](const vector<string>& args) {//FIXME
  /*
  if (args.size() == 0) { not_found(); return; }
  int id;
  if (!read(args[0],id)) { not_found(); return; }
  Global::lock_attempts();
  auto it = Global::attempts.find(id);
  if (it == Global::attempts.end()) {
    Global::unlock_attempts();
    not_found();
    return;
  }
  JSON tmp(it->second);
  Global::unlock_attempts();
  auto& user = tmp("username").str();
  if (user != castsess().user.username) { not_found(); return; }
  string fn = "attempts";
  fn += "/"+tmp("id").str();
  fn += "/"+tmp("problem").str();
  fn += tmp("language").str();
  file(fn,"text/plain");
  */
},true);

// =============================================================================
// POST
// =============================================================================

route("/login",[=](const vector<string>&) {
  if (session()) { location("/"); return; }
  auto& data = payload();
  data.push_back(0);
  JSON json;
  json.parse(&data[0]);
  if (!json("username") || !json("password")) {
    response("Invalid username/password!");
    return;
  }
  User::Data user = User::login(json["username"],json["password"]);
  if (!user.id) {
    response("Invalid username/password!");
    return;
  }
  session(new Session(ip(),move(user)));
  response("ok");
},false,true);

route("/new_attempt",[=](const vector<string>& args) {
  int probid;
  if (!read(args[0],probid)) { not_found(); return; }
  int langid;
  if (!read(args[1],langid)) { not_found(); return; }
  payload().push_back('\n');
  response(Attempt::create(move(JSON(map<string,JSON>{
    {"user"     , castsess().user.id},
    {"problem"  , probid},
    {"language" , langid},
    {"when"     , when()},
    {"ip"       , HTTP::iptostr(ip())}
  })),payload()));
},true,true,2);

} // Handler()

void not_found() {
  location("/");
}

void unauthorized() {
  location("/");
}

bool check_session() {
  if (eqip(castsess().ip,ip())) return false;
  time_t now = time(nullptr);
  tm ti;
  char buf[26];
  strftime(buf,26,"At %H:%M:%S on %d/%m/%Y",localtime_r(&now,&ti));
  log(stringf(
    "%s: %s had a session with IP %s and made a request with IP %s.",
    buf,
    castsess().user.username.c_str(),
    HTTP::iptostr(castsess().ip).c_str(),
    HTTP::iptostr(ip()).c_str()
  ));
  return true;
}

};

static JSON settings;
static bool quit = false;
static pthread_t webserver;
static void* thread(void*) {
  HTTP::server(settings,[&]() { return !quit; },[]() { return new Handler; });
}

namespace WebServer {

void init() {
  uint16_t default_port = 8000;
  unsigned default_nthreads = 0;
  if (!settings.read_file("webserver.json")) settings = JSON();
  uint16_t p;
  if (!settings("port") || !settings("port").read(p)) {
    p = default_port;
  }
  unsigned n;
  if (!settings("client_threads") || !settings("client_threads").read(n)) {
    n = default_nthreads;
  }
  settings["port"] = p;
  settings["client_threads"] = n;
  pthread_create(&webserver,nullptr,thread,nullptr);
}

void close() {
  quit = true;
  pthread_join(webserver,nullptr);
}

uint16_t port() {
  return settings["port"];
}

} // namespace WebServer
