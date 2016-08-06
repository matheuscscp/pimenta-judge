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

#include "global.hpp"

using namespace std;

typedef function<char(char, char*, char*, int, int&)> Script;
struct QueueData {
  Attempt att;
  string lang;
  string path;
  string fn;
};

static map<string, Script> scripts;
static queue<QueueData> jqueue;
static pthread_mutex_t judger_mutex = PTHREAD_MUTEX_INITIALIZER;

static void judge(
  Attempt& att, const string& lang, const string& path, const string& fn
) {
  // run
  int maxms = 0;
  att.verdict = scripts[lang](
    att.problem,
    (char*)path.c_str(),
    (char*)fn.c_str(),
    Global::settings("contest","problems",att.problem-'A',"timelimit"),
    maxms
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

static string valid_filename(JSON& contest, const string& fn) {
  if (!(('A' <= fn[0]) && (fn[0] <= ('A' + contest("problems").size()-1)))) {
    return "";
  }
  auto& langs = contest("languages");
  string lang = fn.substr(1,fn.size());
  return (langs.find(lang) != langs.end_o() ? lang : "");
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
    judge(qd.att,qd.lang,qd.path,qd.fn);
  }
}

string attempt(
  const string& file_name,
  const vector<uint8_t>& file,
  Attempt att
) {
  JSON contest(move(Global::settings("contest")));
  time_t begin = contest("begin");
  time_t end = contest("end");
  
  // check time
  if (att.when < begin || end <= att.when) {
    return "The contest is not running.";
  }
  att.when = int(round((att.when-begin)/60.0));
  
  // check file name
  string lang = valid_filename(contest,file_name);
  if (lang == "") return "Invalid programming language!";
  att.problem = file_name[0];
  
  // set status
  if (contest("problems",att.problem-'A',"autojudge")) att.status = "judged";
  else att.status = "tojudge";
  
  // generate id
  Global::lock_nextid_file();
  att.id = genid();
  Global::unlock_nextid_file();
  
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
  fwrite(&file[0], file.size(), 1, fp);
  fclose(fp);
  
  // push task
  QueueData qd;
  qd.att = att;
  qd.lang = lang;
  qd.path = path;
  qd.fn = fn;
  pthread_mutex_lock(&judger_mutex);
  jqueue.push(qd);
  pthread_mutex_unlock(&judger_mutex);
  
  return "Attempt "+to<string>(att.id)+" received!";
}

} // namespace Judge
