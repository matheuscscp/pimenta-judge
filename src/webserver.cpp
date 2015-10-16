#include <cstring>
#include <string>
#include <map>
#include <set>

#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "webserver.h"

#include "global.h"
#include "judge.h"
#include "scoreboard.h"
#include "clarification.h"

using namespace std;

static string findname(const string& team) {
  FILE* fp = fopen("teams.txt", "r");
  if (!fp) return "";
  string name;
  for (fgetc(fp); name == "" && !feof(fp); fgetc(fp)) {
    string ntmp;
    for (char c = fgetc(fp); c != '"'; c = fgetc(fp)) ntmp += c;
    fgetc(fp);
    string tmp;
    for (char c = fgetc(fp); c != ' '; c = fgetc(fp)) tmp += c;
    string pass;
    for (char c = fgetc(fp); c != '\n'; c = fgetc(fp)) pass += c;
    if (team == tmp) name = ntmp;
  }
  fclose(fp);
  return name;
}

struct Request {
  string line, uri, login, teamname;
  map<string, string> headers;
  Request(int sd) {
    // read socket
    vector<char> buf; {
      char c;
      for (bool done = false; !done;) {
        while (!done && read(sd, &c, 1) == 1) {
          buf.push_back(c);
          if (c == '\n' && buf.size() >= 4 && buf[buf.size()-4] == '\r') {
            done = true;
          }
        }
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
    
    // session cookie
    string cookie = headers["Cookie"];
    auto f = cookie.find("sessionToken=");
    if (f != string::npos) {
      stringstream ss;
      ss << cookie.substr(cookie.find("sessionToken=")+13, cookie.size());
      ss >> login;
      if (login[login.size()-1] == ';') login = login.substr(0, login.size()-1);
      teamname = findname(login);
    }
  }
};

struct Client {
  sockaddr_in addr;
  int sd;
};

static string login(const string& team, const string& password) {
  FILE* fp = fopen("teams.txt", "r");
  if (!fp) return "";
  string name;
  for (fgetc(fp); name == "" && !feof(fp); fgetc(fp)) {
    string ntmp;
    for (char c = fgetc(fp); c != '"'; c = fgetc(fp)) ntmp += c;
    fgetc(fp);
    string tmp;
    for (char c = fgetc(fp); c != ' '; c = fgetc(fp)) tmp += c;
    string pass;
    for (char c = fgetc(fp); c != '\n'; c = fgetc(fp)) pass += c;
    if (team == tmp && password == pass) name = ntmp;
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
    ctype = "application/octet-stream";
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
  Request req(cptr->sd);
  
  // login
  if (req.teamname == "") {
    if (req.line[0] == 'P' && req.line.find("login") != string::npos) {
      string team = req.headers["Team"];
      if (login(team, req.headers["Password"]) == "") {
        write(cptr->sd, "Invalid team/password!", 22);
      }
      else {
        //log(team, cptr->addr.sin_addr.s_addr);//TODO
        string response =
          "HTTP/1.1 200 OK\r\n"
          "Connection: close\r\r"
          "Set-Cookie: sessionToken="+team+"; Max-Age=2592000\r\n"
          "\r\n"
          "k"
        ;
        write(cptr->sd, response.c_str(), response.size());
      }
    }
    else {
      file(cptr->sd, req.uri, "/login.html");
    }
  }
  // logout
  else if (req.line.find("logout") != string::npos) {
    string response =
      "HTTP/1.1 303 See Other\r\n"
      "Connection: close\r\r"
      "Location: /\r\n"
      "Set-Cookie: sessionToken=deleted; Max-Age=0\r\n"
      "\r\n"
    ;
    write(cptr->sd, response.c_str(), response.size());
  }
  else {
    // post
    if (req.line[0] == 'P') {
      if (req.uri.find("attempt") != string::npos) {
        Judge::attempt(
          cptr->sd,
          req.teamname, req.login,
          req.headers["File-name"], to<int>(req.headers["File-size"])
        );
      }
      else if (req.uri.find("question") != string::npos) {
        Clarification::question(
          cptr->sd, req.login, req.headers["Problem"], req.headers["Question"]
        );
      }
    }
    // data request
    else if (req.uri.find("?") != string::npos) {
      if (req.uri.find("teamname") != string::npos) {
        write(cptr->sd, req.teamname.c_str(), req.teamname.size());
      }
      else if (req.uri.find("problems") != string::npos) {
        Settings settings;
        string ans = to<string>(settings.problems.size());
        write(cptr->sd, ans.c_str(), ans.size());
      }
      else if (req.uri.find("scoreboard") != string::npos) {
        Scoreboard::send(cptr->sd);
      }
      else if (req.uri.find("clarifications") != string::npos) {
        Clarification::send(cptr->sd, req.login);
      }
      else if (req.uri.find("statement") != string::npos) {
        statement(cptr->sd);
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
  fcntl(sd, F_SETFL, FNDELAY);
  
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
    socklen_t alen;
    Client c;
    c.sd = accept(sd, (sockaddr*)&c.addr, &alen);
    if (c.sd < 0) { usleep(25000); continue; }
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
