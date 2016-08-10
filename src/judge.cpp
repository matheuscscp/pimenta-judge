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

// %p = path
// %s = source
// %P = problem
static string command(
  const string& srccmd,
  const string& path,
  const string& prob,
  const string& lang
) {
  string cmd;
  for (const char* s = srccmd.c_str(); *s; s++) {
    if (*s != '%') { cmd += *s; continue; }
    s++;
    switch (*s) {
      case 'p': cmd += path;                break;
      case 's': cmd += path+"/"+prob+lang;  break;
      case 'P': cmd += prob;                break;
      default:  cmd += *(--s);              break;
    }
  }
  return cmd;
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

static char run(const string& cmd, int tls, int mlkB, int& mtms, int& mmkB) {
  // init time
  timeval start;
  gettimeofday(&start,nullptr);
  // child
  pid_t pid = fork();
  if (!pid) {
    setpgid(0,0); // create new process group rooted at pid
    rlimit r;
    r.rlim_cur = RLIM_INFINITY;
    r.rlim_max = RLIM_INFINITY;
    setrlimit(RLIMIT_STACK,&r);
    execl("/bin/sh","/bin/sh","-c",cmd.c_str(),(char*)0);
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
  mtms = dt(start)/1000;
  mmkB = r.ru_maxrss;
  if (mtms > tls*1000) return TLE;
  if (mmkB > mlkB) return MLE;
  if (!WIFEXITED(st) || WEXITSTATUS(st) || WIFSIGNALED(st)) return RTE;
  return AC;
}

static void judge(QueueData& qd) {
  Attempt& att = *qd.att;
  
  // get compilation settings
  JSON language(move(Global::settings("contest","languages",att.language)));
  
  // compile
  if (language.find_tuple("compile")) {
    int status = system(command(
      language("compile"),
      qd.path,
      att.problem,
      att.language
    ).c_str());
    if (WEXITSTATUS(status)) {
      att.verdict = CE;
      att.judged = true; // CE needs no judgement by humans
      Attempt::commit(qd.att);
      return;
    }
  }
  
  // get execution settings
  string cmd = move(command(language("run"),qd.path,att.problem,att.language));
  JSON problem(move(Global::settings("contest","problems",att.problem)));
  int Mtms=0,MmkB=0,mtms,mmkB;
  int tls =
    problem.find_tuple(att.language,"timelimit") ?
    problem(att.language,"timelimit") :
    problem("timelimit")
  ;
  int mlkB =
    problem.find_tuple(att.language,"memlimit") ?
    problem(att.language,"memlimit") :
    problem("memlimit")
  ;
  
  // init attempt
  att.verdict = AC;
  att.judged = problem("autojudge");
  
  // for each input file
  string dn = "problems/"+att.problem;
  DIR* dir = opendir((dn+"/input").c_str());
  for (dirent* ent = readdir(dir); ent; ent = readdir(dir)) {
    string fn = ent->d_name;
    string ifn = dn+"/input/"+fn;
    
    // check if dirent is regular file
    struct stat stt;
    stat(ifn.c_str(),&stt);
    if (!S_ISREG(stt.st_mode)) continue;
    
    // run
    string ofn = qd.path+"/output/"+fn;
    att.verdict = run(cmd+" < "+ifn+" > "+ofn,tls,mlkB,mtms,mmkB);
    Mtms = max(Mtms,mtms);
    MmkB = max(MmkB,mmkB);
    if (att.verdict != AC) break;
    
    // diff
    string sfn = dn+"/output/"+fn;
    int status = system("diff -wB %s %s",ofn.c_str(),sfn.c_str());
    if (WEXITSTATUS(status)) { att.verdict = WA; break; }
    status = system("diff %s %s",ofn.c_str(),sfn.c_str());
    if (WEXITSTATUS(status)) { att.verdict = PE; break; }
  }
  
  // commit
  att.time = move(stringf("%d",mtms));
  att.memory = move(stringf("%d",mmkB));
  Attempt::commit(qd.att);
}

namespace Judge {

void* thread(void*) {
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
  auto it = Global::attempts.rbegin();
  if (it != Global::attempts.rend()) att.id = it->first+1;
  Global::attempts[att.id] = move(att.json());
  Global::unlock_attempts();
  
  // save file
  string path = "attempts";
  mkdir(path.c_str(), 0777);
  path += ("/"+att.username);
  mkdir(path.c_str(), 0777);
  path += ("/"+att.problem);
  mkdir(path.c_str(), 0777);
  path += ("/"+to<string>(att.id));
  mkdir(path.c_str(), 0777);
  mkdir((path+"/output").c_str(), 0777);
  FILE* fp = fopen((path+"/"+fn).c_str(), "wb");
  fwrite(&file[0], file.size(), 1, fp);
  fclose(fp);
  
  // push task
  pthread_mutex_lock(&judge_mutex);
  jqueue.emplace(attptr,path);
  pthread_mutex_unlock(&judge_mutex);
  
  return "Attempt "+to<string>(att.id)+" received!";
}

} // namespace Judge
