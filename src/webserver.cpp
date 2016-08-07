#include "webserver.hpp"

#include "global.hpp"
#include "httpserver.hpp"
#include "judge.hpp"
#include "runlist.hpp"
#include "scoreboard.hpp"
#include "clarification.hpp"

using namespace std;

static void log(const string& msg) {
  FILE* fp = fopen("log.txt","a");
  fprintf(fp,"%s\n",msg.c_str());
  fclose(fp);
}

static bool eqip(uint32_t a, uint32_t b) {
  if ((a == 0 || a == 0x0100007f) && (b == 0 || b == 0x0100007f)) return true;
  return a == b;
}

static string find_fullname(const string& username, const string& password) {
  JSON users(move(Global::settings("users")));
  JSON* user = (JSON*)users.find_tuple(username);
  if (!user || (*user)("password").str() != password) return "";
  return (*user)("fullname");
}

class Session : public HTTP::Session {
  public:
    uint32_t ip;
    string username,fullname;
    Session(uint32_t ip, const string& u, const string& f) :
    ip(ip), username(u), fullname(f) {
      uint32_t oip = -1;
      destroy([&](const HTTP::Session* sess) {
        auto& ref = *(Session*)sess;
        if (username != ref.username) return false;
        oip = ref.ip;
        return true;
      });
      if (oip == -1) return;
      time_t now = time(nullptr);
      tm ti;
      char buf[20];
      strftime(buf,20,"%H:%M:%S %d/%m/%Y",localtime_r(&now,&ti));
      log(stringf(
        "%s at %s: IP %s logged in while IP %s was already logged in.",
        username.c_str(),
        buf,
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

route("/login",[=](const vector<string>&) {
  if (session() || method() != "POST") { location("/"); return; }
  auto& data = payload();
  data.push_back(0);
  JSON json;
  json.parse(&data[0]);
  if (!json.find_tuple("username") || !json.find_tuple("password")) {
    response("Invalid username/password!");
    return;
  }
  string fullname = find_fullname(json["username"],json["password"]);
  if (fullname == "") {
    response("Invalid username/password!");
    return;
  }
  session(new Session(ip(),json["username"],fullname));
  response("ok");
});

route("/logout",[=](const vector<string>&) {
  session(nullptr);
  response("ok");
});

route("/attempt",[=](const vector<string>&) {
  if (method() != "POST") { location("/"); return; }
  Attempt* att  = new Attempt;
  att->username = castsess().username;
  att->ip       = HTTP::iptostr(ip());
  response(Judge::attempt(header("file-name"),payload(),att,when()));
},true);

route("/question",[=](const vector<string>&) {
  if (method() != "POST") { location("/"); return; }
  response(Clarification::question(
    castsess().username,header("problem"),header("question")
  ));
},true);

route("/runlist",[=](const vector<string>&) {
  response(Runlist::query(castsess().username),"application/json");
},true);

route("/scoreboard",[=](const vector<string>&) {
  response(Scoreboard::query(),"application/json");
},true);

route("/clarifications",[=](const vector<string>&) {
  json(Clarification::query(castsess().username));
},true);

route("/statement",[=](const vector<string>&) {
  if (time(nullptr)<time_t(Global::settings("contest","begin"))) unauthorized();
  else attachment("statement.pdf");
},true);

route("/status",[=](const vector<string>&) {
  JSON contest(move(Global::settings("contest")));
  time_t begin  = contest("begin");
  time_t end    = contest("end");
  time_t freeze = contest("freeze");
  time_t now    = time(nullptr);
  for (auto& p : contest("problems").arr()) p.erase("autojudge");
  json(map<string,JSON>{
    {"fullname" , castsess().fullname},
    {"rem_time" , now < begin ? 0 : max(0,int(end-now))},
    {"freeze"   , int((end-freeze)/60)},
    {"languages", move(contest("languages"))},
    {"problems" , move(contest("problems"))}
  });
},true);

}

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
  char buf[20];
  strftime(buf,20,"%H:%M:%S %d/%m/%Y",localtime_r(&now,&ti));
  log(stringf(
    "%s at %s: IP %s tried to hack IP %s.",
    castsess().username.c_str(),
    buf,
    HTTP::iptostr(ip()).c_str(),
    HTTP::iptostr(castsess().ip).c_str()
  ));
  return true;
}

};

namespace WebServer {

void* thread(void*) {
  HTTP::server(
    Global::alive, Global::settings("webserver"), []() { return new Handler; }
  );
}

} // namespace WebServer
