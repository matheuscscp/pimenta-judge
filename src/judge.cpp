#include <cstring>
#include <cmath>
#include <functional>
#include <queue>

#include <unistd.h>
#include <dirent.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include "judge.hpp"

#include "global.hpp"

using namespace std;

typedef function<char(char, char*, char*, int, int&)> Script;
struct QueueData {
  Attempt* attptr;
  string lang;
  string path;
  string fn;
};

static map<string, Script> scripts;
static queue<QueueData> jqueue;
static pthread_mutex_t judger_mutex = PTHREAD_MUTEX_INITIALIZER;

static void judge(
  Attempt* attptr, const string& lang, const string& path, const string& fn
) {
  Attempt& att = *attptr;
  char prob = att.problem[0];
  
  // run
  int maxms = 0;
  char verd = scripts[lang](
    prob,
    (char*)path.c_str(),
    (char*)fn.c_str(),
    Global::settings("contest","problems",prob-'A',"timelimit"),
    maxms
  );
  att.runtime = stringf("%.3lf",maxms/1000.0);
  
  // diff files
  if (verd == AC) {
    string dn = stringf("problems/%c/output/",prob);
    DIR* dir = opendir(dn.c_str());
    for (dirent* ent = readdir(dir); ent && verd == AC; ent = readdir(dir)) {
      string fn = dn+ent->d_name;
      
      // check if dirent is regular file
      struct stat stt;
      stat(fn.c_str(),&stt);
      if (!S_ISREG(stt.st_mode)) continue;
      
      // diffs
      int status = system(
        "diff -wB %s %soutput/%s", fn.c_str(), path.c_str(), ent->d_name
      );
      if (WEXITSTATUS(status)) verd = WA;
      else {
        status = system(
          "diff %s %soutput/%s", fn.c_str(), path.c_str(), ent->d_name
        );
        if (WEXITSTATUS(status)) verd = PE;
      }
    }
    closedir(dir);
  }
  att.verdict = verd;
  
  // status
  att.judged = Global::settings("contest","problems",prob-'A',"autojudge");
  
  // save attempt info
  Global::lock_attempts();
  Global::attempts(to<string>(att.id)) = move(att.json());
  Global::unlock_attempts();
  delete attptr;
}

static string valid_filename(const string& fn) {
  JSON contest(move(Global::settings("contest")));
  if (!(('A' <= fn[0]) && (fn[0] <= ('A' + contest("problems").size()-1)))) {
    return "";
  }
  auto& langs = contest("languages").obj();
  string lang = fn.substr(1,fn.size());
  return (langs.find(lang) != langs.end() ? lang : "");
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
  scripts[".c"] = [](char p, char* path, char* fn, int tl, int& ms) {
    int status = system("gcc -std=c11 %s -o %s%c -lm", fn, path, p);
    if (WEXITSTATUS(status)) return char(CE);
    return run(tl,stringf("%s%c",path,p),p,path,ms);
  };
  scripts[".cpp"] = [](char p, char* path, char* fn, int tl, int& ms) {
    int status = system("g++ -std=c++1y %s -o %s%c", fn, path, p);
    if (WEXITSTATUS(status)) return char(CE);
    return run(tl,stringf("%s%c",path,p),p,path,ms);
  };
  scripts[".java"] = [](char p, char* path, char* fn, int tl, int& ms) {
    int status = system("javac %s", fn);
    if (WEXITSTATUS(status)) return char(CE);
    return run(tl,stringf("java -cp %s %c",path,p),p,path,ms);
  };
  scripts[".py"] = [](char p, char* path, char* fn, int tl, int& ms) {
    return run(tl,stringf("python %s", fn),p,path,ms);
  };
  scripts[".py3"] = [](char p, char* path, char* fn, int tl, int& ms) {
    return run(tl,stringf("python3 %s",fn),p,path,ms);
  };
}

namespace Judge {

void* thread(void*) {
  load_scripts();
  while (Global::alive()) {
    pthread_mutex_lock(&judger_mutex);
    if (jqueue.empty()) {
      pthread_mutex_unlock(&judger_mutex);
      usleep(25000);
      continue;
    }
    QueueData qd = jqueue.front(); jqueue.pop();
    pthread_mutex_unlock(&judger_mutex);
    judge(qd.attptr,qd.lang,qd.path,qd.fn);
  }
}

string attempt(
  const string& file_name,
  const vector<uint8_t>& file,
  Attempt* attptr,
  time_t when
) {
  Attempt& att = *attptr;
  time_t begin = Global::settings("contest","begin");
  time_t end   = Global::settings("contest","end");
  
  // check time
  if (when < begin || end <= when) return "The contest is not running.";
  att.when = int(round((when-begin)/60.0));
  
  // check file name
  string lang = valid_filename(file_name);
  if (lang == "") return "Invalid programming language!";
  att.problem = to<string>(file_name[0]);
  
  // generate id
  att.id = 1;
  Global::lock_attempts();
  auto& atts = Global::attempts.obj();
  auto it = atts.rbegin();
  if (it != atts.rend()) att.id = to<int>(it->first)+1;
  atts[move(to<string>(att.id))] = move(att.json());
  Global::unlock_attempts();
  
  // save file
  string fn = "attempts/";
  mkdir(fn.c_str(), 0777);
  fn += (att.username+"/");
  mkdir(fn.c_str(), 0777);
  fn += (att.problem+"/");
  mkdir(fn.c_str(), 0777);
  fn += (to<string>(att.id)+"/");
  mkdir(fn.c_str(), 0777);
  mkdir((fn+"output/").c_str(), 0777);
  string path = "./"+fn;
  fn += file_name;
  FILE* fp = fopen(fn.c_str(), "wb");
  fwrite(&file[0], file.size(), 1, fp);
  fclose(fp);
  
  // push task
  QueueData qd;
  qd.attptr = attptr;
  qd.lang = lang;
  qd.path = path;
  qd.fn = fn;
  pthread_mutex_lock(&judger_mutex);
  jqueue.push(qd);
  pthread_mutex_unlock(&judger_mutex);
  
  return "Attempt "+to<string>(att.id)+" received!";
}

} // namespace Judge
