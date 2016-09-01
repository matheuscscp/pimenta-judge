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

class Session : public HTTP::Session {
  public:
    uint32_t ip;
    int uid;
    Session(uint32_t ip, int uid) : ip(ip), uid(uid) {
      uint32_t oip = -1;
      destroy([&](const HTTP::Session* sess) {
        auto& ref = *(Session*)sess;
        if (uid != ref.uid) return false;
        oip = ref.ip;
        return true;
      });
      if (oip == -1) return;
      time_t now = time(nullptr);
      tm ti;
      char buf[26];
      strftime(buf,26,"At %H:%M:%S on %d/%m/%Y",localtime_r(&now,&ti));
      log(stringf(
        "%s, user id=%d: IP %s logged in while IP %s was already logged in.",
        buf,
        uid,
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
  json(map<string,JSON>{
    {"time" , time(nullptr)},
    {"name" , User::get(castsess().uid)["name"]}
  });
},true);

route("/contests",[=](const vector<string>& args) {
  if (args.size() < 2) { json(Contest::page()); return; }
  unsigned page, page_size;
  if (!read(args[0],page) || !read(args[1],page_size)) {
    json(Contest::page());
    return;
  }
  json(Contest::page(page,page_size));
},true);

route("/problems",[=](const vector<string>& args) {
  if (args.size() < 2) { json(Problem::page(castsess().uid)); return; }
  unsigned page, page_size;
  if (!read(args[0],page) || !read(args[1],page_size)) {
    json(Problem::page(castsess().uid));
    return;
  }
  json(Problem::page(castsess().uid,page,page_size));
},true);

route("/attempts",[=](const vector<string>& args) {
  if (args.size() < 2) { json(Attempt::page(castsess().uid)); return; }
  unsigned page, page_size;
  if (!read(args[0],page) || !read(args[1],page_size)) {
    json(Attempt::page(castsess().uid));
    return;
  }
  json(Attempt::page(castsess().uid,page,page_size));
},true);

route("/logout",[=](const vector<string>&) {
  session(nullptr);
  location("/");
},true);

route("/contest",[=](const vector<string>& args) {
  int cid;
  if (!read(args[0],cid)) { not_found(); return; }
  json(Contest::get(cid,castsess().uid));
},true,false,1);

route("/contest/problems",[=](const vector<string>& args) {
  int cid;
  if (!read(args[0],cid)) { not_found(); return; }
  json(Contest::get_problems(cid,castsess().uid));
},true,false,1);

route("/contest/attempts",[=](const vector<string>& args) {
  int cid;
  if (!read(args[0],cid)) { not_found(); return; }
  json(Contest::get_attempts(cid,castsess().uid));
},true,false,1);

route("/contest/scoreboard",[=](const vector<string>& args) {
  int cid;
  if (!read(args[0],cid)) { not_found(); return; }
  json(Contest::scoreboard(cid,castsess().uid));
},true,false,1);

route("/problem",[=](const vector<string>& args) {
  int probid;
  if (!read(args[0],probid)) { not_found(); return; }
  json(Problem::get(probid,castsess().uid));
},true,false,1);

route("/problem/statement",[=](const vector<string>& args) {
  int probid;
  if (!read(args[0],probid)) { not_found(); return; }
  string fn = Problem::statement(probid,castsess().uid);
  if (fn != "") file(fn);
},true,false,1);

route("/attempt",[=](const vector<string>& args) {
  int id;
  if (!read(args[0],id)) { not_found(); return; }
  json(Attempt::get(id,castsess().uid));
},true,false,1);

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
  int uid = User::login(json["username"],json["password"]);
  if (!uid) {
    response("Invalid username/password!");
    return;
  }
  session(new Session(ip(),uid));
  response("ok");
},false,true);

route("/new_attempt",[=](const vector<string>& args) {
  int probid;
  if (!read(args[0],probid)) { not_found(); return; }
  payload().push_back('\n');
  response(Attempt::create(move(JSON(map<string,JSON>{
    {"user"     , castsess().uid},
    {"problem"  , probid},
    {"language" , args[1]},
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
