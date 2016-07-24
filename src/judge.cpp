#include <cstring>
#include <cmath>
#include <set>
#include <map>
#include <functional>
#include <queue>

#include <unistd.h>
#include <dirent.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include "judge.hpp"

#define BSIZ (1 << 18)

using namespace std;

typedef function<char(char, char*, char*, Settings&, int&)> Script;
struct QueueData {
  Attempt att;
  string lang;
  string path;
  string fn;
  Settings settings;
};

static map<string, Script> scripts;
static queue<QueueData> jqueue;
static pthread_mutex_t judger_mutex = PTHREAD_MUTEX_INITIALIZER;

static void judge(
  Attempt& att,
  const string& lang, const string& path, const string& fn,
  Settings& settings
) {
  // run
  int maxms = 0;
  att.verdict = scripts[lang](
    att.problem, (char*)path.c_str(), (char*)fn.c_str(), settings, maxms
  );
  att.runtime = stringf("%.3lf",maxms/1000.0);
  
  // diff files
  if (att.verdict == AC) {
    string dn = stringf("problems/%c/output/",att.problem);
    DIR* dir = opendir(dn.c_str());
    for (
      dirent* ent = readdir(dir);
      ent && att.verdict == AC;
      ent = readdir(dir)
    ) {
      string fn = dn+ent->d_name;
      
      // check if dirent is regular file
      struct stat stt;
      stat(fn.c_str(),&stt);
      if (!S_ISREG(stt.st_mode)) continue;
      
      // diffs
      int status = system(
        "diff -wB %s %soutput/%s", fn.c_str(), path.c_str(), ent->d_name
      );
      if (WEXITSTATUS(status)) att.verdict = WA;
      else {
        status = system(
          "diff %s %soutput/%s", fn.c_str(), path.c_str(), ent->d_name
        );
        if (WEXITSTATUS(status)) att.verdict = PE;
      }
    }
    closedir(dir);
  }
  
  // save attempt info
  Global::lock_att_file();
  FILE* fp = fopen("attempts.txt", "a");
  att.write(fp);
  fclose(fp);
  Global::unlock_att_file();
}

static string valid_filename(Settings& settings, const string& fn) {
  if (!(('A' <= fn[0]) && (fn[0] <= ('A' + int(settings.problems.size())-1)))) {
    return "";
  }
  string lang = fn.substr(1,fn.size());
  return (settings.langs.find(lang) != settings.langs.end() ? lang : "");
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

static __suseconds_t dt(const timeval& start) {
  timeval end;
  gettimeofday(&end, nullptr);
  return
    (end.tv_sec*1000000 + end.tv_usec)-
    (start.tv_sec*1000000 + start.tv_usec)
  ;
}
static char run(
  int tle_secs,
  const string& exec_cmd,
  char problem,
  const string& output_path,
  int& ms
) {
  char ans = AC;
  __suseconds_t us = tle_secs*1000000;
  
  string dn = stringf("problems/%c/input/",problem);
  DIR* dir = opendir(dn.c_str());
  for (dirent* ent = readdir(dir); ent && ans == AC; ent = readdir(dir)) {
    string fn = dn+ent->d_name;
    
    // check if dirent is regular file
    struct stat stt;
    stat(fn.c_str(),&stt);
    if (!S_ISREG(stt.st_mode)) continue;
    
    // init time
    timeval start;
    gettimeofday(&start, nullptr);
    
    // child
    pid_t proc = fork();
    if (!proc) {
      // TODO lose root permissions
      setpgid(0, 0); // create new process group rooted at proc
      int status = system(
        "%s < %s > %soutput/%s",
        exec_cmd.c_str(),
        fn.c_str(),
        output_path.c_str(),
        ent->d_name
      );
      exit(WEXITSTATUS(status));
    }
    
    // judge
    int status;
    while (waitpid(proc, &status, WNOHANG) != proc && ans == AC) {
      if (dt(start) > us) {
        kill(-proc, SIGKILL); // the minus kills the whole group rooted at proc
        waitpid(proc, &status, 0);
        ans = TLE;
      }
      if (ans == AC) usleep(10000);
    }
    ms = (ans == TLE ? tle_secs*1000 : max(ms,int(dt(start)/1000)));
    if (WEXITSTATUS(status)) ans = RTE;
  }
  closedir(dir);
  
  return ans;
}
static void load_scripts() {
  scripts[".c"] = [](char p, char* path, char* fn, Settings& sets, int& ms) {
    int status = system("gcc -std=c11 %s -o %s%c -lm", fn, path, p);
    if (WEXITSTATUS(status)) return char(CE);
    return run(
      sets.problems[p-'A'].timelimit,
      stringf("%s%c", path, p),
      p,
      path,
      ms
    );
  };
  scripts[".cpp"] = [](char p, char* path, char* fn, Settings& sets, int& ms) {
    int status = system("g++ -std=c++1y %s -o %s%c", fn, path, p);
    if (WEXITSTATUS(status)) return char(CE);
    return run(
      sets.problems[p-'A'].timelimit,
      stringf("%s%c", path, p),
      p,
      path,
      ms
    );
  };
  scripts[".java"] = [](char p, char* path, char* fn, Settings& sets, int& ms) {
    int status = system("javac %s", fn);
    if (WEXITSTATUS(status)) return char(CE);
    return run(
      sets.problems[p-'A'].timelimit,
      stringf("java -cp %s %c", path, p),
      p,
      path,
      ms
    );
  };
  scripts[".py"] = [](char p, char* path, char* fn, Settings& sets, int& ms) {
    return run(
      sets.problems[p-'A'].timelimit,
      stringf("python %s", fn),
      p,
      path,
      ms
    );
  };
  scripts[".py3"] = [](char p, char* path, char* fn, Settings& sets, int& ms) {
    return run(
      sets.problems[p-'A'].timelimit,
      stringf("python3 %s", fn),
      p,
      path,
      ms
    );
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
    QueueData qd = jqueue.front(); jqueue.pop();
    pthread_mutex_unlock(&judger_mutex);
    judge(qd.att,qd.lang,qd.path,qd.fn,qd.settings);
  }
  return nullptr;
}

namespace Judge {

void fire() {
  load_scripts();
  Global::fire(judger);
}

void attempt(int sd, const std::string& file_name, int file_size, Attempt att) {
  Settings settings;
  
  // check time
  if (att.when < settings.begin || settings.end <= att.when) {
    if (file_size) ignoresd(sd);
    write(sd, "The contest is not running.", 27);
    return;
  }
  att.when = int(round((att.when-settings.begin)/60.0));
  
  // check file name
  string lang = valid_filename(settings,file_name);
  if (lang == "") {
    if (file_size) ignoresd(sd);
    write(sd, "Invalid programming language!", 29);
    return;
  }
  att.problem = file_name[0];
  
  // set status
  if (settings.problems[att.problem-'A'].autojudge) att.status = "judged";
  else att.status = "tojudge";
  
  // check file size
  if (file_size > BSIZ) {
    string resp =
      "Files with more than "+to<string>(BSIZ)+" bytes are not allowed!"
    ;
    if (file_size) ignoresd(sd);
    write(sd, resp.c_str(), resp.size());
    return;
  }
  
  // read data
  char* buf = new char[BSIZ];
  for (int i = 0, fs = file_size; fs > 0;) {
    int rb = read(sd, &buf[i], fs);
    if (rb < 0) {
      write(sd, "File corrupted!", 15);
      delete[] buf;
      return;
    }
    fs -= rb;
    i += rb;
  }
  
  // generate id
  Global::lock_nextid_file();
  att.id = genid();
  Global::unlock_nextid_file();
  
  // respond
  string response = "Attempt "+to<string>(att.id)+" received!";
  write(sd, response.c_str(), response.size());
  
  // save file
  string fn = "attempts/";
  mkdir(fn.c_str(), 0777);
  fn += (att.username+"/");
  mkdir(fn.c_str(), 0777);
  fn += att.problem; fn += "/";
  mkdir(fn.c_str(), 0777);
  fn += (to<string>(att.id)+"/");
  mkdir(fn.c_str(), 0777);
  mkdir((fn+"output/").c_str(), 0777);
  string path = "./"+fn;
  fn += file_name;
  FILE* fp = fopen(fn.c_str(), "wb");
  fwrite(buf, file_size, 1, fp);
  fclose(fp);
  delete[] buf;
  
  // push task
  QueueData qd;
  qd.att = att;
  qd.lang = lang;
  qd.path = path;
  qd.fn = fn;
  qd.settings = settings;
  pthread_mutex_lock(&judger_mutex);
  jqueue.push(qd);
  pthread_mutex_unlock(&judger_mutex);
}

} // namespace Judge
