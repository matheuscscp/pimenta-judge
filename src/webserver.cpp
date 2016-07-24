#include <cstring>
#include <string>
#include <map>
#include <set>

#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "webserver.hpp"

#include "global.hpp"
#include "judge.hpp"
#include "runlist.hpp"
#include "scoreboard.hpp"
#include "clarification.hpp"

#define P3P \
"P3P: CP=\"CURa ADMa DEVa PSAo PSDo OUR BUS UNI PUR INT" \
"DEM STA PRE COM NAV OTC NOI DSP COR\"\r\n"

using namespace std;

struct Client {
  int sd;
  uint32_t ip;
  int when;
};

struct Session {
  string token;
  uint32_t ip;
  time_t end;
  string username;
  string teamname;
  bool expired() const { return end <= time(nullptr); }
};

static map<string, Session> sessions;
static map<string, string> session_tokens;
static pthread_mutex_t sessions_mutex = PTHREAD_MUTEX_INITIALIZER;

static void log(const string& msg) {
  FILE* fp = fopen("log.txt","a");
  fprintf(fp,"%s\n",msg.c_str());
  fclose(fp);
}

static bool eqip(uint32_t a, uint32_t b) {
  if ((a == 0 || a == 0x0100007f) && (b == 0 || b == 0x0100007f)) return true;
  return a == b;
}
static Session get_session(const string& token, uint32_t ip) {
  Session session;
  pthread_mutex_lock(&sessions_mutex);
  auto it = sessions.find(token);
  if (it != sessions.end()) {
    if (!it->second.expired() && eqip(it->second.ip,ip)) session = it->second;
    else {
      if (!eqip(it->second.ip,ip)) { // hacking attempt detected
        time_t now = time(nullptr);
        tm ti;
        char buf[20];
        strftime(buf,20,"%d/%m/%Y %H:%M:%S",localtime_r(&now,&ti));
        log(stringf(
          "%s at %s: IP %s tried to hack IP %s.",
          it->second.username.c_str(),
          buf,
          to<string>(ip).c_str(),
          to<string>(it->second.ip).c_str()
        ));
      }
      session_tokens.erase(it->second.username);
      sessions.erase(it);
    }
  }
  pthread_mutex_unlock(&sessions_mutex);
  return session;
}

static char tohex(char num) {
  if (num < 10) return num+'0';
  num -= 10;
  return num+'a';
}
static string generate_token() {
  string ans;
  for (int i = 0; i < 16; i++) {
    char buf = 0;
    for (int j = 0; j < 8; j++) if (rand()%100 < 50) {
      buf |= (1<<j);
    }
    ans += tohex((buf>>4)&0x0f);
    ans += tohex(buf&0x0f);
  }
  return ans;
}
static void create_session(Session& session) {
  pthread_mutex_lock(&sessions_mutex);
  // generate token
  do {
    session.token = generate_token();
  } while (sessions.find(session.token) != sessions.end());
  // detect already existing session for this account
  auto it = session_tokens.find(session.username);
  if (it != session_tokens.end()) {
    auto it2 = sessions.find(it->second);
    if (!it2->second.expired()) { // detected
      time_t now = time(nullptr);
      tm ti;
      char buf[20];
      strftime(buf,20,"%d/%m/%Y %H:%M:%S",localtime_r(&now,&ti));
      log(stringf(
        "%s at %s: IP %s logged in while IP %s was already logged in.",
        session.username.c_str(),
        buf,
        to<string>(session.ip).c_str(),
        to<string>(it2->second.ip).c_str()
      ));
    }
    sessions.erase(it2);
    it->second = session.token;
  }
  else session_tokens[session.username] = session.token;
  sessions[session.token] = session;
  pthread_mutex_unlock(&sessions_mutex);
}

static void remove_session(const string& token) {
  pthread_mutex_lock(&sessions_mutex);
  auto it = sessions.find(token);
  if (it != sessions.end()) {
    session_tokens.erase(it->second.username);
    sessions.erase(it);
  }
  pthread_mutex_unlock(&sessions_mutex);
}

static void clean_sessions() {
  // delay cleanups
  static time_t next = 0;
  time_t now = time(nullptr);
  if (now < next) return;
  next = now + 600;
  
  // clean
  pthread_mutex_lock(&sessions_mutex);
  for (auto it = sessions.begin(); it != sessions.end();) {
    if (!it->second.expired()) { it++; continue; }
    session_tokens.erase(it->second.username);
    sessions.erase(it++);
  }
  pthread_mutex_unlock(&sessions_mutex);
}

struct Request {
  string line, uri;
  map<string, string> headers;
  Request(const Client& cl, Session& session) {
    // read socket
    vector<char> buf; {
      bool done = false;
      for (char c; !done && read(cl.sd,&c,1) == 1;) {
        buf.push_back(c);
        if (c == '\n' && buf.size() >= 4 && buf[buf.size()-4] == '\r') {
          done = true;
        }
      }
      if (!done) {
        line = "badrequest";
        return;
      }
    }
    int i = 0;
    
    // parse request line
    for (; buf[i] != '\r'; i++) line += buf[i];
    i += 2;
    
    // fetch uri
    stringstream ss;
    ss << line;
    ss >> uri;
    ss >> uri;
    
    // parse headers
    while (true) {
      string line;
      for (; buf[i] != '\r'; i++) line += buf[i];
      i += 2;
      if (line == "") break;
      int j = 0;
      string header;
      for (; j < int(line.size()) && line[j] != ':'; j++) header += line[j];
      string content;
      for (j += 2; j < int(line.size()); j++) content += line[j];
      headers[header] = content;
    }
    
    // get session
    string cookie = headers["Cookie"];
    auto f = cookie.find("sessionToken=");
    if (f != string::npos) {
      stringstream ss;
      ss << cookie.substr(cookie.find("sessionToken=")+13, cookie.size());
      string token;
      ss >> token;
      if (token[token.size()-1] == ';') token = token.substr(0,token.size()-1);
      session = get_session(token,cl.ip);
    }
  }
};

static string find_teamname(const string& username, const string& password) {
  FILE* fp = fopen("teams.txt", "r");
  if (!fp) return "";
  string name;
  for (fgetc(fp); name == "" && !feof(fp); fgetc(fp)) {
    string ntmp;
    for (char c = fgetc(fp); c != '"' && !feof(fp); c = fgetc(fp)) ntmp += c;
    fgetc(fp);
    string tmp;
    for (char c = fgetc(fp); c != ' ' && !feof(fp); c = fgetc(fp)) tmp += c;
    string pass;
    for (char c = fgetc(fp); c != '\n' && !feof(fp); c = fgetc(fp)) pass += c;
    if (username == tmp && password == pass) name = ntmp;
  }
  fclose(fp);
  return name;
}

static void file(int sd, string uri, const string& def) {
  string notfound =
    "<html>\n"
      "<body>\n"
        "<h1>Not Found</h1>\n"
      "</body>\n"
    "</html>\n"
  ;
  string ctype;
  if (uri.find(".html") != string::npos) {
    ctype = "text/html";
  }
  else if (uri.find(".css") != string::npos) {
    ctype = "text/css";
  }
  else if (uri.find(".js") != string::npos) {
    ctype = "application/javascript";
  }
  else if (uri.find(".gif") != string::npos) {
    ctype = "image/gif";
  }
  else if (uri.find(".svg") != string::npos) {
    ctype = "image/svg+xml";
  }
  else {
    uri = def;
    ctype = "text/html";
  }
  FILE* fp = fopen(("www"+uri).c_str(), "rb");
  if (!fp) ctype = "text/html";
  string header =
    "HTTP/1.1 200 OK\r\n"
    "Connection: close\r\r"
    "Content-Type: "+ctype+"\r\n"
    "\r\n"
  ;
  write(sd, header.c_str(), header.size());
  if (!fp) {
    write(sd, notfound.c_str(), notfound.size());
    return;
  }
  char* buf = new char[1<<10];
  int sz;
  while ((sz = fread(buf, 1, 1<<10, fp)) > 0) write(sd, buf, sz);
  delete[] buf;
  fclose(fp);
}

static void statement(int sd) {
  Settings settings;
  string response;
  bool send = false;
  if (time(nullptr) < settings.begin) {
    response =
      "HTTP/1.1 403 Forbidden\r\n"
      "Connection: close\r\r"
      "\r\n"
    ;
  }
  else {
    response =
      "HTTP/1.1 200 OK\r\n"
      "Connection: close\r\r"
      "Content-Type: application/octet-stream\r\n"
      "Content-Disposition: attachment; filename=\"statement.pdf\"\r\n"
      "\r\n"
    ;
    send = true;
  }
  write(sd, response.c_str(), response.size());
  if (!send) return;
  FILE* fp = fopen("statement.pdf", "rb");
  if (!fp) return;
  char* buf = new char[1<<20];
  for (size_t rd = 0; (rd = fread(buf, 1, 1 << 20, fp)) > 0;) {
    write(sd, buf, rd);
  }
  delete[] buf;
  fclose(fp);
}

static void* client(void* ptr) {
  Client* cptr = (Client*)ptr;
  Session sess;
  Request req(*cptr,sess);
  if (req.line == "badrequest") {
    close(cptr->sd);
    delete cptr;
    return nullptr;
  }
  
  // login
  if (sess.username == "") {
    if (req.line[0] == 'P' && req.uri.find("login") != string::npos) {
      sess.username = req.headers["Team"];
      sess.teamname = find_teamname(sess.username,req.headers["Password"]);
      if (sess.teamname == "") write(cptr->sd, "Invalid team/password!", 22);
      else {
        sess.ip = cptr->ip;
        sess.end = time(nullptr) + 2592000;
        create_session(sess);
        string response =
          "HTTP/1.1 200 OK\r\n"
          "Connection: close\r\r"
          P3P
          "Set-Cookie: sessionToken="+sess.token+"; Max-Age=2592000\r\n"
          "\r\n"
          "ok"
        ;
        write(cptr->sd, response.c_str(), response.size());
      }
    }
    else {
      file(cptr->sd, req.uri, "/login.html");
    }
  }
  // logout
  else if (req.uri.find("logout") != string::npos) {
    remove_session(sess.token);
    string response =
      "HTTP/1.1 200 OK\r\n"
      "Connection: close\r\r"
      P3P
      "Set-Cookie: sessionToken=deleted; Max-Age=0\r\n"
      "\r\n"
      "ok"
    ;
    write(cptr->sd, response.c_str(), response.size());
  }
  else {
    // post
    if (req.line[0] == 'P') {
      if (req.uri.find("attempt") != string::npos) {
        Attempt att;
        att.when = cptr->when;
        att.username = sess.username;
        att.ip = to<string>(cptr->ip);
        att.teamname = sess.teamname;
        Judge::attempt(
          cptr->sd,
          req.headers["File-name"], to<int>(req.headers["File-size"]),
          att
        );
      }
      else if (req.uri.find("question") != string::npos) {
        Clarification::question(
          cptr->sd,
          sess.username,
          req.headers["Problem"],
          req.headers["Question"]
        );
      }
    }
    // data request
    else if (req.uri.find("?") != string::npos) {
      if (req.uri.find("runlist") != string::npos) {
        Runlist::send(sess.username,cptr->sd);
      }
      else if (req.uri.find("scoreboard") != string::npos) {
        Scoreboard::send(cptr->sd,Settings().freeze <= time(nullptr));
      }
      else if (req.uri.find("clarifications") != string::npos) {
        Clarification::send(sess.username,cptr->sd);
      }
      else if (req.uri.find("statement") != string::npos) {
        statement(cptr->sd);
      }
      else if (req.uri.find("teamname") != string::npos) {
        write(cptr->sd, sess.teamname.c_str(), sess.teamname.size());
      }
      else if (req.uri.find("problems") != string::npos) {
        string ans = to<string>(Settings().problems.size());
        write(cptr->sd, ans.c_str(), ans.size());
      }
      else if (req.uri.find("remaining-time") != string::npos) {
        Settings settings;
        time_t now = time(nullptr);
        int tmp = (now < settings.begin ? 0 : max(0,int(settings.end-now)));
        string ans = to<string>(tmp);
        write(cptr->sd, ans.c_str(), ans.size());
      }
      else if (req.uri.find("enabled-langs") != string::npos) {
        string ans = Settings().enabled_langs();
        write(cptr->sd, ans.c_str(), ans.size());
      }
      else if (req.uri.find("limits") != string::npos) {
        string ans = Settings().limits();
        write(cptr->sd, ans.c_str(), ans.size());
      }
    }
    // file request
    else file(cptr->sd, req.uri, "/index.html");
  }
  
  close(cptr->sd);
  delete cptr;
  return nullptr;
}

static void* server(void*) {
  // create socket
  int sd = socket(AF_INET, SOCK_STREAM, 0);
  int opt = 1;
  setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
  fcntl(sd, F_SETFL, O_NONBLOCK);
  
  // set addr
  sockaddr_in addr;
  memset(&addr, 0, sizeof addr);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(to<uint16_t>(Global::arg[2]));
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  bind(sd, (sockaddr*)&addr, sizeof addr);
  
  // listen
  listen(sd, SOMAXCONN);
  while (Global::alive()) {
    clean_sessions();
    Client c;
    sockaddr_in addr;
    socklen_t alen;
    c.sd = accept(sd, (sockaddr*)&addr, &alen);
    if (c.sd < 0) { usleep(25000); continue; }
    c.ip = addr.sin_addr.s_addr;
    c.when = time(nullptr);
    pthread_t thread;
    pthread_create(&thread, nullptr, client, new Client(c));
    pthread_detach(thread);
  }
  
  // close
  close(sd);
  return nullptr;
}

namespace WebServer {

void fire() {
  Global::fire(server);
}

} // namespace WebServer
