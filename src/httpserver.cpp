#include <cstring>
#include <fstream>
#include <queue>
#include <list>
#include <algorithm>

#include <unistd.h>
#include <fcntl.h>
#include <semaphore.h>
#include <netinet/in.h>
#include <sys/stat.h>

#include "httpserver.hpp"

using namespace std;

// helpers
static bool hasws(const string& s) {
  for (char c : s) if (isspace(c)) return true;
  return false;
}
static bool istchar(char c) {
  switch (c) {
    case  '!':
    case  '#':
    case  '$':
    case  '%':
    case  '&':
    case '\'':
    case  '*':
    case  '+':
    case  '-':
    case  '^':
    case  '_':
    case  '`':
    case  '|':
    case  '~':
      return true;
  }
  return isdigit(c) || isalpha(c);
}
static bool istoken(const string& s) {
  for (char c : s) if (!istchar(c)) return false;
  return s.size() > 0;
}
static bool ispchar(char c) {
  switch (c) {
    case  '-':
    case  '.':
    case  '_':
    case  '~':
    case  '%':
    case  '!':
    case  '$':
    case  '&':
    case '\'':
    case  '(':
    case  ')':
    case  '*':
    case  '+':
    case  ',':
    case  ';':
    case  '=':
    case  ':':
    case  '@':
      return true;
  }
  return isalpha(c) || isdigit(c);
}
static unsigned char fromhex(const char* s) {
  char ans = 0;
  for (int i = 0; i < 2; i++) {
    ans|=((s[i]<='9'?s[i]-'0':(s[i]<='f' ? s[i]-'a':s[i]-'A')+10)<<((1-i)<<2));
  }
  return ans;
}
static string ext(const string& fn) {
  int dot;
  for (dot = fn.size()-1; 0 <= dot; dot--) if (fn[dot] == '.') break;
  if (dot < 0) return "";
  string ans = move(fn.substr(dot+1,string::npos));
  transform(ans.begin(),ans.end(),ans.begin(),::tolower);
  return ans;
}
static bool ishex(const string& s) {
  for (char c : s) if (!isxdigit(c)) return false;
  return true;
}
static string hexstr(unsigned x) {
  string ans = "00000000";
  sprintf((char*)ans.c_str(),"%08x",x);
  return ans;
}

// settings
static JSON settings;
template <typename T, typename... Args>
T setting(T def, Args... args) {
  auto ptr = settings.find_tuple(args...);
  if (!ptr) return def;
  T ans;
  if (!ptr->to(ans)) return def;
  return ans;
}

// client threads
static sem_t queue_sem;
static queue<HTTP::Handler*> conn_queue;
static pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static void* thread(void*) {
  while (true) {
    sem_wait(&queue_sem);
    pthread_mutex_lock(&queue_mutex);
    HTTP::Handler* handler = conn_queue.front(); conn_queue.pop();
    pthread_mutex_unlock(&queue_mutex);
    if (!handler) break;
    handler->handle();
    delete handler;
  }
}

// sessions
struct SessionEntry {
  time_t end;
  HTTP::Session* sess;
  SessionEntry() : sess(nullptr) {}
};
static map<unsigned,SessionEntry> sessions;
static pthread_mutex_t sessions_mutex = PTHREAD_MUTEX_INITIALIZER;
static HTTP::Session* load_session(unsigned& id) {
  pthread_mutex_lock(&sessions_mutex);
  auto it = sessions.find(id);
  if (it == sessions.end()) {
    do {
      id = 0;
      for (int shf = 0; shf < 32; shf += 8) id |= ((rand()&0xffu)<<shf);
    } while (!id || sessions.find(id) != sessions.end());
    it = sessions.emplace(id,SessionEntry()).first;
  }
  it->second.end = time(nullptr)+2592000;
  HTTP::Session* ans = (it->second.sess ? it->second.sess->clone() : nullptr);
  pthread_mutex_unlock(&sessions_mutex);
  return ans;
}
static void store_session(unsigned id, HTTP::Session* sess) {
  pthread_mutex_lock(&sessions_mutex);
  auto& sent = sessions[id];
  delete sent.sess;
  sent.sess = sess;
  pthread_mutex_unlock(&sessions_mutex);
}
static void clean_sessions() {
  static const int period = setting(86400,"session","clean_period");
  static time_t next = 0;
  time_t now = time(nullptr);
  if (now < next) return;
  next = now+period;
  pthread_mutex_lock(&sessions_mutex);
  for (auto it = sessions.begin(); it != sessions.end();) {
    if (now < it->second.end) { it++; continue; }
    delete it->second.sess;
    sessions.erase(it++);
  }
  pthread_mutex_unlock(&sessions_mutex);
}

namespace HTTP {

Session::~Session() {}

Handler::Handler() {
  routes["/"] = [=](const vector<string>& segments) {
    string fn;
    if (segments.size() > 0) {
      if (segments[0] == "..") { notfound(); return; }
      fn = segments[0];
      for (int i = 1; i < segments.size(); i++) {
        if (segments[i] == "..") { notfound(); return; }
        fn += "/"+segments[i];
      }
    }
    if (fn == "") {
      if (ifstream("index.html").is_open()) file("index.html");
      else response("<h1>HTTP Server OK</h1>","text/html");
    }
    else if (!ifstream(fn).is_open()) notfound();
    else file(fn);
  };
}

Handler::~Handler() {
  if (st_code) { // request not processed
    stringstream ss;
    ss << st_version << " " << st_code << " " << st_phrase << "\r\n\r\n";
    string resp = move(ss.str());
    write(sd,resp.c_str(),resp.size());
  }
  shutdown(sd,SHUT_WR);
  close(sd);
}

void Handler::route(
  const string& path,
  const function<void(const vector<string>&)>& cb
) {
  routes[path] = cb;
}

void Handler::notfound() {
  status(404,"Not Found");
  response("<h1>Not Found</h1>","text/html");
}

time_t Handler::when() const {
  return when_;
}

uint32_t Handler::ip() const {
  return ip_;
}

string& Handler::method() {
  return method_;
}

const string& Handler::method() const {
  return method_;
}

string& Handler::uri() {
  return uri_;
}

const string& Handler::uri() const {
  return uri_;
}

string& Handler::version() {
  return version_;
}

const string& Handler::version() const {
  return version_;
}

string& Handler::path() {
  return path_;
}

const string& Handler::path() const {
  return path_;
}

string& Handler::query() {
  return query_;
}

const string& Handler::query() const {
  return query_;
}

string Handler::header(const string& name) const {
  auto it = req_headers.find(name);
  if (it == req_headers.end()) return "";
  return it->second;
}

vector<uint8_t>& Handler::payload() {
  return payload_;
}

const vector<uint8_t>& Handler::payload() const {
  return payload_;
}

void Handler::status(int code, const string& phrase, const string& version) {
  st_version = version;
  st_code = code;
  st_phrase = phrase;
}

void Handler::header(const string& name, const string& value) {
  resp_headers[name] = value;
}

void Handler::response(const string& d, const string& type) {
  data = d;
  isfile = false;
  if (data == "") {
    resp_headers.erase("Content-Length");
    resp_headers.erase("Content-Type");
    return;
  }
  stringstream ss;
  ss << data.size();
  resp_headers["Content-Length"] = move(ss.str());
  resp_headers["Content-Type"] = (type == "" ? "text/plain" : type);
}

void Handler::response(const JSON& json) {
  response(json.generate(),"application/json");
}

void Handler::file(const string& path, const string& type) {
  data = path;
  isfile = true;
  FILE* fp = fopen(data.c_str(),"rb");
  if (!fp) {
    data = "";
    isfile = false;
    resp_headers.erase("Content-Length");
    resp_headers.erase("Content-Type");
    return;
  }
  fclose(fp);
  struct stat st;
  stat(data.c_str(),&st);
  stringstream ss;
  ss << st.st_size;
  resp_headers["Content-Length"] = move(ss.str());
  if (type != "") { resp_headers["Content-Type"] = type; return; }
  static const map<string,string> exts{
    {"html" , "text/html"},
    {"css"  , "text/css"},
    {"js"   , "application/javascript"},
    {"json" , "application/json"},
    {"bmp"  , "image/bmp"},
    {"gif"  , "image/gif"},
    {"png"  , "image/png"},
    {"jpg"  , "image/jpeg"},
    {"jpeg" , "image/jpeg"},
    {"svg"  , "image/svg+xml"},
    {"pdf"  , "application/pdf"}
  };
  auto it = exts.find(ext(data));
  if (it != exts.end()) resp_headers["Content-Type"] = it->second;
  else resp_headers["Content-Type"] = "application/octet-stream";
}

Session* Handler::session() const {
  return sess;
}

void Handler::session(Session* nsess) {
  delete sess;
  sess = nsess;
}

void Handler::init(int sd, time_t when, uint32_t ip) {
  // socket
  this->sd = sd;
  when_ = when;
  ip_ = ip;
  // socket options
  static const time_t timeout = setting(0,"request","timeout");
  timeval tv;
  memset(&tv,0,sizeof tv);
  tv.tv_sec = timeout;
  setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
  // status line
  st_code = 200;
  st_phrase = "OK";
  st_version = "HTTP/1.1";
  // headers
  resp_headers["Connection"] = "close";
  // response
  data = "";
  isfile = false;
}

void Handler::handle() {
  // request line
  {
    static const int max_size = setting(1024,"request","max_uri_size");
    if (!getline(max_size)) return;
    if (buf.size() == max_size) { status(414,"Request-URI Too Long"); return; }
    if (buf == "") { status(400,"Bad Request"); return; }
    // method
    auto del = buf.find(' ');
    if (!del || del == string::npos) { status(400,"Bad Request"); return; }
    method_ = move(buf.substr(0,del));
    buf.erase(0,del+1);
    if (!istoken(method_)) { status(400,"Bad Request"); return; }
    // URI
    del = buf.find(' ');
    if (!del || del == string::npos) { status(400,"Bad Request"); return; }
    uri_ = move(buf.substr(0,del));
    buf.erase(0,del+1);
    if (hasws(uri_)) { status(400,"Bad Request"); return; }
    // version
    version_ = move(buf);
    if (version_.size()!= 8|| !isdigit(version_[5]) || !isdigit(version_[7])) {
      status(400,"Bad Request");
      return;
    }
    char mj = version_[5], mn = version_[7];
    version_[5] = '0'; version_[7] = '0';
    if (version_ != "HTTP/0.0") { status(400,"Bad Request"); return; }
    version_[5] = mj; version_[7] = mn;
  }
  
  // request headers
  for (int headers = 0; ; headers++) {
    static const int max_size = setting(1024,"request","max_header_size");
    static const int max_headers = setting(1024,"request","max_headers");
    // stop conditions
    if (!getline(max_size)) return;
    if (buf == "") break; // done
    if (buf.size() == max_size || headers == max_headers) {
      status(431,"Request Header Fields Too Large");
      return;
    }
    // name
    auto colon = buf.find(':');
    if (!colon || colon == string::npos) { status(400,"Bad Request"); return; }
    string name = move(buf.substr(0,colon));
    buf.erase(0,colon+1);
    if (!istoken(name)) { status(400,"Bad Request"); return; }
    transform(name.begin(),name.end(),name.begin(),::tolower);
    // value
    int l = 0, r = buf.size()-1;
    while (buf[l] == ' ' || buf[l] == '\t') l++;
    while (0 <= r && (buf[r] == ' ' || buf[r] == '\t')) r--;
    string value;
    if (r < l) value = "1";
    else value = move(buf.substr(l,r-l+1));
    req_headers[move(name)] = move(value);
  }
  
  // message body (handling only content-length header)
  {
    static const int max_size = setting(1048576,"request","max_payload_size");
    int clen;
    auto it = req_headers.find("content-length");
    if (it != req_headers.end() && sscanf(it->second.c_str(),"%d",&clen) == 1) {
      if (sscanf(it->second.c_str(),"%d",&clen) != 1) {
        status(411,"Length Required");
        return;
      }
      if (clen > max_size) { status(413,"Request Entity Too Large"); return; }
      uint8_t buf;
      int sysret;
      for (int i = 0; i < clen; i++) {
        if ((sysret = read(sd,&buf,1)) == 1) payload_.push_back(buf);
        else if (sysret == 0) { status(400,"Bad Request"); return; }
        else { status(408,"Request Timeout"); return; }
      }
    }
  }
  
  // load session
  unsigned sid = 0;
  {
    string cookie = req_headers["cookie"];
    auto it = cookie.find("sessionID=");
    if (it != string::npos) {
      cookie = move(cookie.substr(it+10,8));
      if (cookie.size() == 8 && ishex(cookie)) sscanf(cookie.c_str(),"%x",&sid);
    }
    sess = load_session(sid);
    resp_headers["Set-Cookie"] = "sessionID="+hexstr(sid)+"; Max-Age=2592000";
    resp_headers["P3P"] = // to allow cookies
      "CP=\"CURa ADMa DEVa PSAo PSDo OUR BUS UNI PUR INT "
      "DEM STA PRE COM NAV OTC NOI DSP COR\""
    ;
  }
  
  // route by path with origin-form syntax rule
  {
    // origin-form
    auto qm = uri_.find('?');
    if (qm == string::npos) qm = uri_.size();
    else query_ = move(uri_.substr(qm+1,string::npos));
    path_ = move(uri_.substr(0,qm));
    // absolute-path
    vector<string> segments;
    for (int i = 0; i < qm; i++) { // for each segment
      if (uri_[i] != '/') { segments.clear(); break; }
      auto nx = uri_.find('/',i+1);
      if (nx == string::npos) nx = qm;
      string segment;
      int j;
      for (j = i+1; j < nx; j++) { // for each pchar
        if (!ispchar(uri_[j])) { segments.clear(); break; }
        if (uri_[j] != '%') { segment += uri_[j]; continue; }
        if (!isxdigit(uri_[j+1])) { segments.clear(); break; }
        if (!isxdigit(uri_[j+2])) { segments.clear(); break; }
        j++; segment += fromhex(&uri_[j++]);
      }
      if (j < nx) break;
      i = nx-1;
      segments.push_back(move(segment));
    }
    // do the routing
    string route;
    int arg;
    for (arg = 0; arg < segments.size(); arg++) {
      string tmp = "/"+segments[arg];
      if (routes.find(route+tmp) == routes.end()) break;
      route += tmp;
    }
    vector<string> args(segments.begin()+arg,segments.end());
    if (route != "") routes[route](args);
    else if (path_ == "/" || segments.size() > 0) routes["/"](args);
    else notfound();
  }
  
  // store session
  store_session(sid,sess);
  
  // response
  stringstream ss;
  ss << st_version << " " << st_code << " " << st_phrase << "\r\n";
  st_code = 0; // destructor will send another status line if st_code != 0
  for (auto& kv : resp_headers) ss << kv.first << ": " << kv.second << "\r\n";
  ss << "\r\n";
  string resp = move(ss.str());
  write(sd,resp.c_str(),resp.size());
  if (method_ == "HEAD") return;
  if (!isfile) {
    if (data != "") write(sd,data.c_str(),data.size());
    return;
  }
  FILE* fp = fopen(data.c_str(),"rb");
  uint8_t* buf = new uint8_t[1<<20];
  for (int sz; (sz = fread(buf,1,1<<20,fp)) > 0; write(sd,buf,sz));
  delete[] buf;
  fclose(fp);
}

bool Handler::getline(int max_size) {
  buf = "";
  int i = 0, sysret;
  for (char c; (sysret = read(sd,&c,1)) == 1 && buf.size() < max_size;) {
    if (c == 0) { status(400,"Bad Request"); return false; }
    buf += c;
    i++;
    if (i > 1 && buf[i-2] == '\r' && buf[i-1] == '\n') {
      buf.erase(i-2,string::npos);
      return true;
    }
  }
  if (buf.size() == max_size) return true;
  if (sysret < 0) status(408,"Request Timeout");
  else status(400,"Bad Request");
  return false;
}

void server(
  function<bool()> alive,
  const JSON& setts,
  function<Handler*()> handler_factory,
  ostream& log
) {
  // read settings
  settings = setts;
  uint16_t port = setting(8000,"port");
  if (port == 8000) log << "httpserver: using default port (8000)\n";
  unsigned nthreads = setting(0,"client_threads");
  if (nthreads == 0) {
    log << "httpserver: using default number of client threads (0)\n";
  }
  else if (nthreads > sysconf(_SC_NPROCESSORS_ONLN)) {
    log << "httpserver: limiting number of client threads to the number of ";
    log << "online processors (" << sysconf(_SC_NPROCESSORS_ONLN) << ")\n";
    nthreads = sysconf(_SC_NPROCESSORS_ONLN);
  }
  
  // create socket
  int ssd = socket(AF_INET, SOCK_STREAM, 0);
  int opt = 1;
  setsockopt(ssd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
  fcntl(ssd, F_SETFL, O_NONBLOCK);
  
  // set server address
  sockaddr_in svaddr;
  memset(&svaddr, 0, sizeof svaddr);
  svaddr.sin_family = AF_INET;
  svaddr.sin_port = htons(port);
  svaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  bind(ssd, (sockaddr*)&svaddr, sizeof svaddr);
  
  // start threads
  sem_init(&queue_sem, 0, 0);
  list<pthread_t> threads;
  for (int i = 0; i < nthreads; i++) {
    threads.emplace_back();
    pthread_create(&threads.back(), nullptr, thread, nullptr);
  }
  
  // listen
  listen(ssd, SOMAXCONN);
  while (alive()) {
    clean_sessions();
    sockaddr_in addr;
    socklen_t addrlen = sizeof addr;
    int sd = accept(ssd, (sockaddr*)&addr, &addrlen);
    time_t when = time(nullptr);
    if (sd < 0) { usleep(25000); continue; }
    Handler* tmp = handler_factory();
    tmp->init(sd,when,addr.sin_addr.s_addr);
    if (threads.size() == 0) {
      tmp->handle();
      delete tmp;
    }
    else {
      pthread_mutex_lock(&queue_mutex);
      conn_queue.push(tmp);
      pthread_mutex_unlock(&queue_mutex);
      sem_post(&queue_sem);
    }
  }
  
  // stop threads
  for (pthread_t& th : threads) {
    pthread_mutex_lock(&queue_mutex);
    conn_queue.push(nullptr);
    pthread_mutex_unlock(&queue_mutex);
    sem_post(&queue_sem);
  }
  for (pthread_t& th : threads) pthread_join(th, nullptr);
  
  // close
  close(ssd);
}

} // namespace HTTP
