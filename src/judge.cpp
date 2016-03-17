#include <cstring>
#include <set>
#include <map>
#include <functional>
#include <queue>

#include <unistd.h>
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
  else {
    bool found = false;
    for (int i = 1; instance_exists(fno[0], i) && !found; i++) {
           if (system("diff -wB %s%c.out%d problems/%c.sol%d", path.c_str(), fno[0], i, fno[0], i)) {
        found = true;
        att.verdict = WA;
      }
      else if (system("diff     %s%c.out%d problems/%c.sol%d", path.c_str(), fno[0], i, fno[0], i)) {
        found = true;
        att.verdict = PE;
      }
    }
    if (!found) att.verdict = AC;
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

static void load_scripts() {
  scripts[".c"] = [](char p, const char* path, const char* fn, Settings& settings) {
    if (system("gcc -std=c11 %s -o %s%c -lm", fn, path, p)) return CE;
    bool tle;
    char cmd[200];
    sprintf(cmd, "%s%c", path, p);
    int status = timeout2(tle, settings.problems[p-'A'], cmd, p, path);
    if (tle) return TLE;
    if (status) return RTE;
    return AC;
  };
  scripts[".cpp"] = [](char p, const char* path, const char* fn, Settings& settings) {
    if (system("g++ -std=c++1y %s -o %s%c", fn, path, p)) return CE;
    bool tle;
    int status = timeout(tle, settings.problems[p-'A'], "%s%c < problems/%c.in > %sout.txt", path, p, p, path);
    if (tle) return TLE;
    if (status) return RTE;
    return AC;
  };
  scripts[".java"] = [](char p, const char* path, const char* fn, Settings& settings) {
    if (system("javac %s", fn)) return CE;
    bool tle;
    int status = timeout(tle, settings.problems[p-'A'], "java -cp %s %c < problems/%c.in > %sout.txt", path, p, p, path);
    if (tle) return TLE;
    if (status) return RTE;
    return AC;
  };
  scripts[".py"] = [](char p, const char* path, const char* fn, Settings& settings) {
    bool tle;
    int status = timeout(tle, settings.problems[p-'A'], "python %s < problems/%c.in > %sout.txt", fn, p, path);
    if (tle) return TLE;
    if (status) return RTE;
    return AC;
  };
  scripts[".py3"] = [](char p, const char* path, const char* fn, Settings& settings) {
    bool tle;
    int status = timeout(tle, settings.problems[p-'A'], "python3 %s < problems/%c.in > %sout.txt", fn, p, path);
    if (tle) return TLE;
    if (status) return RTE;
    return AC;
  };
  scripts[".cs"] = [](char p, const char* path, const char* fn, Settings& settings) {
    if (system("mcs %s", fn)) return CE;
    bool tle;
    int status = timeout(tle, settings.problems[p-'A'], "%s%c.exe < problems/%c.in > %sout.txt", path, p, p, path);
    if (tle) return TLE;
    if (status) return RTE;
    return AC;
  };
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

namespace Judge {

void fire() {
  load_scripts();
  Global::fire(judger);
}

void attempt(
  int sd,
  const string& teamname, const string& team,
  const string& file_name, int file_size
) {
  Settings settings;
  
  // check time
  time_t now = time(nullptr);
  if (now < settings.begin || settings.end <= now) {
    ignoresd(sd);
    write(sd, "The contest is not running.", 27);
    return;
  }
  
  // check file name
  if (!valid_filename(settings, file_name)) {
    ignoresd(sd);
    write(sd, "Invalid file name!", 18);
    return;
  }
  
  // check file size
  if (file_size > BSIZ) {
    string resp =
      "Files with more than "+to<string>(BSIZ)+" bytes are not allowed!"
    ;
    ignoresd(sd);
    write(sd, resp.c_str(), resp.size());
    return;
  }
  
  // read data
  char* buf = new char[BSIZ];
  for (int i = 0, fs = file_size; fs > 0;) {
    int rb = read(sd, &buf[i], fs);
    if (rb < 0) {
      write(sd, "Incomplete request!", 19);
      delete[] buf;
      return;
    }
    fs -= rb;
    i += rb;
  }
  
  // generate id
  Global::lock_nextid_file();
  if (!Global::alive()) {
    Global::unlock_nextid_file();
    delete[] buf;
    return;
  }
  int id = genid();
  Global::unlock_nextid_file();
  
  // save file
  string fn = "attempts/";
  mkdir(fn.c_str(), 0777);
  fn += (team+"/");
  mkdir(fn.c_str(), 0777);
  fn += file_name[0]; fn += "/";
  mkdir(fn.c_str(), 0777);
  fn += (to<string>(id)+"/");
  mkdir(fn.c_str(), 0777);
  string path = "./"+fn;
  fn += file_name;
  FILE* fp = fopen(fn.c_str(), "wb");
  fwrite(buf, file_size, 1, fp);
  fclose(fp);
  delete[] buf;
  
  // respond
  string response = "Attempt "+to<string>(id)+": ";
  QueueData qd;
  qd.id = id;
  qd.team = teamname;
  qd.fno = file_name;
  qd.path = path;
  qd.fn = fn;
  qd.settings = settings;
  qd.push();
  response += qd.verdict;
  write(sd, response.c_str(), response.size());
}

} // namespace Judge
