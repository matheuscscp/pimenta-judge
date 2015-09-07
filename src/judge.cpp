#include <cstring>
#include <fstream>
#include <set>
#include <map>
#include <functional>
#include <queue>

#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>

#include "judge.h"

#include "global.h"

#define BSIZ (1 << 18)

using namespace std;

typedef function<char(char, const char*, const char*, Settings&)> Script;
struct QueueData;

static map<string, Script> scripts;
static queue<QueueData*> jqueue;
static pthread_mutex_t judger_mutex = PTHREAD_MUTEX_INITIALIZER;

static string judge(
  int id, const string& team, const string& fno,
  const string& path, const string& fn, Settings& settings
) {
  // build attempt
  Attempt att;
  att.id = id;
  strcpy(att.team, team.c_str());
  att.problem = fno[0]-'A';
  char run = scripts[fno.substr(1, fno.size())](
    fno[0], path.c_str(), fn.c_str(), settings
  );
  if (run != AC) {
    att.verdict = run;
  }
  else if (system("diff -wB %sout.txt problems/%c.sol", path.c_str(), fno[0])) {
    att.verdict = WA;
  }
  else if (system("diff     %sout.txt problems/%c.sol", path.c_str(), fno[0])) {
    att.verdict = PE;
  }
  else {
    att.verdict = AC;
  }
  att.when = time(nullptr);
  
  // save attempt info
  Global::lock_att_file();
  if (Global::alive()) {
    FILE* fp = fopen("attempts.bin", "ab");
    fwrite(&att, sizeof att, 1, fp);
    fclose(fp);
  }
  Global::unlock_att_file();
  
  // return verdict
  static string verdict[] = {"AC", "CE", "RTE", "TLE", "WA", "PE"};
  return
    att.when < settings.noverdict ?
    verdict[int(att.verdict)] :
    "The judge is hiding verdicts!"
  ;
}

struct QueueData {
  int id;
  string team;
  string fno;
  string path;
  string fn;
  Settings settings;
  string verdict;
  bool done;
  QueueData() : done(false) {}
  void push() {
    pthread_mutex_lock(&judger_mutex);
    jqueue.push(this);
    pthread_mutex_unlock(&judger_mutex);
    while (Global::alive() && !done) usleep(25000);
  }
  void judge() {
    verdict = ::judge(id, team, fno, path, fn, settings);
    done = true;
  }
};

static string password(const string& team) {
  fstream f("teams.txt");
  if (!f.is_open()) return "";
  string tmp, pass;
  while (f >> tmp >> pass) if (tmp == team) return pass;
  return "";
}

static bool valid_filename(Settings& settings, const string& fn) {
  return
    ('A' <= fn[0]) && (fn[0] <= ('A' + int(settings.problems.size())-1)) &&
    (scripts.find(fn.substr(1, fn.size())) != scripts.end())
  ;
}

static int genid() {
  FILE* fp = fopen("nextid.bin", "rb");
  int current, next;
  if (!fp) current = 1, next = 2;
  else {
    fread(&current, sizeof current, 1, fp);
    fclose(fp);
    next = current + 1;
  }
  fp = fopen("nextid.bin", "wb");
  fwrite(&next, sizeof next, 1, fp);
  fclose(fp);
  return current;
}

static void handle_attempt(int sd, char* buf, Settings& settings) {
  // parse headers
  set<string> what_headers;
  what_headers.insert("Team:");
  what_headers.insert("Password:");
  what_headers.insert("File-name:");
  what_headers.insert("File-size:");
  map<string, string> headers;
  stringstream ss; ss << buf;
  string tmp;
  while (ss >> tmp) {
    if (what_headers.find(tmp) != what_headers.end()) {
      string t2;
      ss >> t2;
      headers[tmp] = t2;
      if (headers.size() == 4) break;
    }
  }
  string pass = password(headers["Team:"]);
  if (pass == "" || pass != headers["Password:"]) {
    ignoresd(sd);
    write(sd, "Invalid team/password!", 22);
    return;
  }
  if (!valid_filename(settings, headers["File-name:"])) {
    ignoresd(sd);
    write(sd, "Invalid file name!", 18);
    return;
  }
  int file_size = to<int>(headers["File-size:"]);
  if (file_size > BSIZ) {
    string resp =
      "Files with more than "+to<string>(BSIZ)+" bytes are not allowed!"
    ;
    ignoresd(sd);
    write(sd, resp.c_str(), resp.size());
    return;
  }
  
  // read data
  for (int i = 0; file_size > 0;) {
    int rb = read(sd, &buf[i], file_size);
    if (rb < 0) {
      write(sd, "Incomplete request!", 19);
      return;
    }
    file_size -= rb;
    i += rb;
  }
  
  // generate id
  Global::lock_nextid_file();
  if (!Global::alive()) {
    Global::unlock_nextid_file();
    return;
  }
  int id = genid();
  Global::unlock_nextid_file();
  
  // save file
  string fn = "attempts/";
  mkdir(fn.c_str(), 0777);
  fn += (headers["Team:"]+"/");
  mkdir(fn.c_str(), 0777);
  fn += headers["File-name:"][0]; fn += "/";
  mkdir(fn.c_str(), 0777);
  fn += (to<string>(id)+"/");
  mkdir(fn.c_str(), 0777);
  string path = "./"+fn;
  fn += headers["File-name:"];
  FILE* fp = fopen(fn.c_str(), "wb");
  fwrite(buf, to<int>(headers["File-size:"]), 1, fp);
  fclose(fp);
  
  // respond
  string response = "Attempt "+to<string>(id)+": ";
  QueueData qd;
  qd.id = id;
  qd.team = headers["Team:"];
  qd.fno = headers["File-name:"];
  qd.path = path;
  qd.fn = fn;
  qd.settings = settings;
  qd.push();
  response += qd.verdict;
  write(sd, response.c_str(), response.size());
}

static void send_form(int sd) {
  string form =
    "HTTP/1.1 200 OK\r\n"
    "Connection: close\r\r"
    "Content-Type: text/html\r\n"
    "\r\n"
    "<html>\n"
    "  <head>\n"
    "    <script>\n"
    "      function sendform() {\n"
    "        response = document.getElementById(\"response\");\n"
    "        response.innerHTML = \"Wait for the verdict.\";\n"
    "        team = document.getElementById(\"team\");\n"
    "        pass = document.getElementById(\"pass\");\n"
    "        file = document.getElementById(\"file\");\n"
    "        if (window.XMLHttpRequest)\n"
    "          xmlhttp = new XMLHttpRequest();\n"
    "        else\n"
    "          xmlhttp = new ActiveXObject(\"Microsoft.XMLHTTP\");\n"
    "        xmlhttp.onreadystatechange = function() {\n"
    "          if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {\n"
    "            response = document.getElementById(\"response\");\n"
    "            response.innerHTML = xmlhttp.responseText;\n"
    "          }\n"
    "        }\n"
    "        xmlhttp.open(\"POST\", \"\", true);\n"
    "        xmlhttp.setRequestHeader(\"Team\", team.value);\n"
    "        xmlhttp.setRequestHeader(\"Password\", pass.value);\n"
    "        xmlhttp.setRequestHeader(\"File-name\", file.files[0].name);\n"
    "        xmlhttp.setRequestHeader(\"File-size\", file.files[0].size);\n"
    "        xmlhttp.send(file.files[0]);\n"
    "        team.value = \"\";\n"
    "        pass.value = \"\";\n"
    "        file.value = \"\";\n"
    "      }\n"
    "    </script>\n"
    "  </head>\n"
    "  <body>\n"
    "    <h1 align=\"center\">Pimenta Judgezzz~*~*</h1>\n"
    "    <h2 align=\"center\">Submission</h2>\n"
    "    <h4 align=\"center\" id=\"response\"></h4>\n"
    "    <table align=\"center\">\n"
    "      <tr>\n"
    "        <td>Team:</td>\n"
    "        <td><input type=\"text\" id=\"team\" autofocus/></td>\n"
    "      </tr>\n"
    "      <tr>\n"
    "        <td>Password:</td>\n"
    "        <td><input type=\"password\" id=\"pass\" /></td>\n"
    "      </tr>\n"
    "      <tr>\n"
    "        <td>File:</td>\n"
    "        <td><input type=\"file\" id=\"file\" /></td>\n"
    "      </tr>\n"
    "      <tr><td><button onclick=\"sendform()\">Send</button></td></tr>\n"
    "    </table>\n"
    "  </body>\n"
    "</html>\n"
  ;
  write(sd, form.c_str(), form.size());
}

static bool read_headers(int sd, char* buf) {
  for (int i = 0, state = 0; read(sd, &buf[i], 1) == 1; i++) {
    switch (state) {
      case 0: {
        state = buf[i] == '\r' ? 1 : 0;
        break;
      }
      case 1: {
        if (buf[i] == '\r') break;
        state = buf[i] == '\n' ? 2 : 0;
        break;
      }
      case 2: {
        state = buf[i] == '\r' ? 3 : 0;
        break;
      }
      case 3: {
             if (buf[i] == '\r') state = 1;
        else if (buf[i] == '\n') {
          buf[i+1] = 0;
          return true;
        }
        else                     state = 0;
      }
    }
  }
  return false;
}

static void* client(void* ptr) {
  // fetch socket
  int* sdptr = (int*)ptr;
  int sd = *sdptr;
  delete sdptr;
  
  char* buf = new char[BSIZ];
  if (!read_headers(sd, buf)) write(sd, "Incomplete request!", 19);
  else if (buf[0] == 'G') send_form(sd);
  else { // post with attempt
    Settings settings;
    time_t now = time(nullptr);
    if (settings.begin <= now && now < settings.end) {
      handle_attempt(sd, buf, settings);
    }
    else {
      ignoresd(sd);
      write(sd, "The contest is not running.", 27);
    }
  }
  
  // close
  delete[] buf;
  close(sd);
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
    int csd = accept(sd, nullptr, nullptr);
    if (csd < 0) { usleep(25000); continue; }
    pthread_t thread;
    pthread_create(&thread, nullptr, client, new int(csd));
    pthread_detach(thread);
  }
  
  // close
  close(sd);
  return nullptr;
}

static void* judger(void*) {
  while (Global::alive()) {
    pthread_mutex_lock(&judger_mutex);
    if (jqueue.empty()) {
      pthread_mutex_unlock(&judger_mutex);
      usleep(25000);
      continue;
    }
    QueueData* qd = jqueue.front(); jqueue.pop();
    pthread_mutex_unlock(&judger_mutex);
    qd->judge();
  }
  return nullptr;
}

static void load_scripts() {
  scripts[".c"] = [](char p, const char* path, const char* fn, Settings& settings) {
    if (system("gcc -std=c11 %s -o %s%c", fn, path, p)) return CE;
    bool tle;
    if (timeout(tle, settings.problems[p-'A'], "%s%c < problems/%c.in > %sout.txt", path, p, p, path)) return RTE;
    if (tle) return TLE;
    return AC;
  };
  scripts[".cpp"] = [](char p, const char* path, const char* fn, Settings& settings) {
    if (system("g++ -std=c++1y %s -o %s%c", fn, path, p)) return CE;
    bool tle;
    if (timeout(tle, settings.problems[p-'A'], "%s%c < problems/%c.in > %sout.txt", path, p, p, path)) return RTE;
    if (tle) return TLE;
    return AC;
  };
  scripts[".java"] = [](char p, const char* path, const char* fn, Settings& settings) {
    if (system("javac %s", fn)) return CE;
    bool tle;
    if (timeout(tle, settings.problems[p-'A'], "java -cp %s %c < problems/%c.in > %sout.txt", path, p, p, path)) return RTE;
    if (tle) return TLE;
    return AC;
  };
  scripts[".py"] = [](char p, const char* path, const char* fn, Settings& settings) {
    bool tle;
    if (timeout(tle, settings.problems[p-'A'], "python %s < problems/%c.in > %sout.txt", fn, p, path)) return RTE;
    if (tle) return TLE;
    return AC;
  };
  scripts[".cs"] = [](char p, const char* path, const char* fn, Settings& settings) {
    if (system("mcs %s", fn)) return CE;
    bool tle;
    if (timeout(tle, settings.problems[p-'A'], "%s%c.exe < problems/%c.in > %sout.txt", path, p, p, path)) return RTE;
    if (tle) return TLE;
    return AC;
  };
}

namespace Judge {

void fire() {
  load_scripts();
  Global::fire(server);
  Global::fire(judger);
}

} // namespace Judge
