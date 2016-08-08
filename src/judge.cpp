#include <cstring>
#include <functional>
#include <queue>

#include <unistd.h>
#include <dirent.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/resource.h>

#include "judge.hpp"

#include "helper.hpp"
#include "global.hpp"

using namespace std;

typedef function<char(char, char*, char*, int, int&)> Script;
struct QueueData {
  Attempt* att;
  string path;
  QueueData(Attempt* att, string path) : att(att), path(path) {}
};

static map<string, Script> scripts;
static queue<QueueData> jqueue;
static pthread_mutex_t judge_mutex = PTHREAD_MUTEX_INITIALIZER;

static void judge(QueueData& qd) {
  Attempt& att = *qd.att;
  JSON problem(move(Global::settings("contest","problems",att.problem)));
  
  // run
  int maxms = 0;
  char verd = scripts[att.language](
    att.problem[0],
    (char*)qd.path.c_str(),
    (char*)(qd.path+att.problem+att.language).c_str(),
    int(problem("timelimit")),
    maxms
  );
  att.runtime = stringf("%.3lf",maxms/1000.0);
  
  // diff files
  if (verd == AC) {
    string dn = stringf("problems/%c/output/",att.problem[0]);
    DIR* dir = opendir(dn.c_str());
    for (dirent* ent = readdir(dir); ent && verd == AC; ent = readdir(dir)) {
      string fn = dn+ent->d_name;
      
      // check if dirent is regular file
      struct stat stt;
      stat(fn.c_str(),&stt);
      if (!S_ISREG(stt.st_mode)) continue;
      
      // diffs
      int status = system(
        "diff -wB %s %soutput/%s", fn.c_str(), qd.path.c_str(), ent->d_name
      );
      if (WEXITSTATUS(status)) verd = WA;
      else {
        status = system(
          "diff %s %soutput/%s", fn.c_str(), qd.path.c_str(), ent->d_name
        );
        if (WEXITSTATUS(status)) verd = PE;
      }
    }
    closedir(dir);
  }
  att.verdict = verd;
  
  // status
  att.judged = problem("autojudge");
  
  // save attempt info
  Global::lock_attempts();
  Global::attempts(to<string>(att.id)) = move(att.json());
  Global::unlock_attempts();
  delete qd.att;
}

static __suseconds_t dtt(const timeval& start) {
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
      if (dtt(start) > us) {
        kill(-proc, SIGKILL); // the minus kills the whole group rooted at proc
        waitpid(proc, &status, 0);
        ans = TLE;
      }
      if (ans == AC) usleep(10000);
    }
    ms = (ans == TLE ? tle_secs*1000 : max(ms,int(dtt(start)/1000)));
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

static long long dt(const timeval& start) {
  timeval end;
  gettimeofday(&end,nullptr);
  long long ans = end.tv_sec;
  ans *= 1000000;
  ans += end.tv_usec;
  ans -= start.tv_usec;
  ans -= 1000000*(long long)start.tv_sec;
  return ans;
}
static char run(const char* c, int tls, int mlkB, int& mtms, int& mmkB) {
  // init time
  timeval start;
  gettimeofday(&start,nullptr);
  // child
  pid_t pid = fork();
  if (!pid) {
    setpgid(0,0); // create new process group rooted at pid
    rlimit r;
    r.rlim_cur = (1<<30)-1;
    r.rlim_max = (1<<30)-1;
    setrlimit(RLIMIT_AS,&r);
    r.rlim_cur = RLIM_INFINITY;
    r.rlim_max = RLIM_INFINITY;
    setrlimit(RLIMIT_STACK,&r);
    execl("/bin/sh","/bin/sh","-c",c,(char*)0);
    _exit(0);
  }
  // parent
  int st;
  rusage r;
  long long tl = tls*(long long)1000000;
  while (wait4(pid,&st,WNOHANG,&r) != pid) {
    if (dt(start) > tl) {
      kill(-pid,SIGKILL); // the minus kills the whole group rooted at pid
      wait4(pid,nullptr,0,&r);
      break;
    }
    usleep(10000);
  }
  mtms = min(tls*1000,int(dt(start)/1000));
  mmkB = min(mlkB,int(r.ru_maxrss));
  if (mtms == tls*1000) return TLE;
  if (mmkB == mlkB) return MLE;
  if (!WIFEXITED(st) || WEXITSTATUS(st) || WIFSIGNALED(st)) return RTE;
  return AC;
}
static char run(
  const string& problem,  // problems/problem/input
  const string& path,     // path/{source|target|output/}
  JSON& settings,         // {language: {...}, problem: {...}}
  int& mtms,
  int& mmkB,
  bool enforce_limits = true
) {
  //TODO
}

namespace Judge {

void* thread(void*) {
  load_scripts();
  while (Global::alive()) {
    pthread_mutex_lock(&judge_mutex);
    if (jqueue.empty()) {
      pthread_mutex_unlock(&judge_mutex);
      usleep(25000);
      continue;
    }
    QueueData qd = jqueue.front(); jqueue.pop();
    pthread_mutex_unlock(&judge_mutex);
    judge(qd);
  }
}

string attempt(const string& fn, const vector<uint8_t>& file, Attempt* attptr) {
  Attempt& att = *attptr;
  JSON contest(move(Global::settings("contest")));
  
  // check file name
  auto dot = fn.find('.');
  if (dot == string::npos) return "Invalid filename!";
  att.problem = move(fn.substr(0,dot));
  if (!contest.find_tuple("problems",att.problem)) return "Invalid filename!";
  att.language = move(fn.substr(dot,string::npos));
  if (!contest.find_tuple("languages",att.language)) {
    return "Invalid programming language!";
  }
  
  // generate id
  att.id = 1;
  Global::lock_attempts();
  auto& atts = Global::attempts.obj();
  auto it = atts.rbegin();
  if (it != atts.rend()) att.id = to<int>(it->first)+1;
  atts[move(to<string>(att.id))] = move(att.json());
  Global::unlock_attempts();
  
  // save file
  string path = "attempts/";
  mkdir(path.c_str(), 0777);
  path += (att.username+"/");
  mkdir(path.c_str(), 0777);
  path += (att.problem+"/");
  mkdir(path.c_str(), 0777);
  path += (to<string>(att.id)+"/");
  mkdir(path.c_str(), 0777);
  mkdir((path+"output/").c_str(), 0777);
  FILE* fp = fopen((path+fn).c_str(), "wb");
  fwrite(&file[0], file.size(), 1, fp);
  fclose(fp);
  
  // push task
  pthread_mutex_lock(&judge_mutex);
  jqueue.emplace(attptr,path);
  pthread_mutex_unlock(&judge_mutex);
  
  return "Attempt "+to<string>(att.id)+" received!";
}

} // namespace Judge
