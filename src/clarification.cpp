#include <cstring>

#include <unistd.h>
#include <sys/stat.h>

#include "clarification.h"

#include "global.h"

using namespace std;

static string buf1, buf2;
static string* frontbuf = &buf1;
static string* backbuf = &buf2;
static pthread_mutex_t frontbuf_mutex = PTHREAD_MUTEX_INITIALIZER;

static void send_page(int sd, Settings& settings) {
  // make local copy of clarifications
  pthread_mutex_lock(&frontbuf_mutex);
  string clarifications(*frontbuf);
  pthread_mutex_unlock(&frontbuf_mutex);
  
  // create select
  string opts;
  for (int i = 0; i < int(settings.problems.size()); i++) {
    string prob = to<string>(char('A'+i));
    opts += ("<option value=\""+prob+"\">"+prob+"</option>\n");
  }
  string select =
    "<select id=\"problem\" autofocus>\n"
    "  <option></option>\n"+
       opts+
    "</select>\n"
  ;
  
  // respond
  string response =
    "HTTP/1.1 200 OK\r\n"
    "Connection: close\r\r"
    "Content-Type: text/html\r\n"
    "\r\n"
    "<h2>Clarifications</h2>\n"
    "<h4 id=\"response\"></h4>\n"
    "<table>\n"
    "  <tr>\n"
    "    <td>Problem:</td>\n"
    "    <td>"+select+"</td>\n"
    "  </tr>\n"
    "  <tr>\n"
    "    <td>Question:</td>\n"
    "    <td><textarea"
    "     id=\"question\""
    "     rows=\"4\" cols=\"50\""
    "    ></textarea></td>\n"
    "  </tr>\n"
    "  <tr><td><button onclick=\"question()\">Send</button></td></tr>\n"
    "</table>\n"
    "<table border=\"3\">\n"
    "  <tr><th>Problem</th><th>Question</th><th>Answer</th></tr>\n"+
       clarifications+
    "</table>\n"
  ;
  write(sd, response.c_str(), response.size());
}

static void create_question(
  int sd, const string& problem, const string& question, Settings& settings
) {
  // check problem
  if (
    problem.size() == 0 || problem.size() > 1 ||
    problem[0] < 'A' || problem[0] > char('A' + settings.problems.size() - 1)
  ) {
    write(sd, "Choose a problem!", 17);
    return;
  }
  
  // check text
  if (question.size() == 0) {
    write(sd, "Write something!", 16);
    return;
  }
  
  // save
  mkdir("questions", 0777);
  char fn[17];
  strcpy(fn, "questions/XXXXXX");
  Global::lock_question_file();
  if (!Global::alive()) {
    Global::unlock_question_file();
    return;
  }
  int fd = mkstemp(fn);
  dprintf(fd, "Problem: %c\nQuestion: %s\n", problem[0], question.c_str());
  close(fd);
  Global::unlock_question_file();
  system("gedit %s &", fn);
}

static void update() {
  struct clarification {
    string str;
    bool read(FILE* fp) {
      char problem;
      if (fscanf(fp, "%c", &problem) != 1) return false;
      str  = "<tr>";
      str += ("<td>"+to<string>(problem)+"</td><td>");
      fgetc(fp);
      fgetc(fp);
      for (char c = fgetc(fp); c != '"' && c != EOF; str += c, c = fgetc(fp));
      fgetc(fp);
      fgetc(fp);
      str += "</td><td>";
      for (char c = fgetc(fp); c != '"' && c != EOF; str += c, c = fgetc(fp));
      fgetc(fp);
      str += "</td></tr>";
      return true;
    }
  };
  
  // update back buffer
  FILE* fp = fopen("clarifications.txt", "r");
  if (!fp) return;
  (*backbuf) = "";
  for (clarification c; c.read(fp); (*backbuf) += c.str);
  fclose(fp);
  
  // swap buffers
  string* tmp = backbuf;
  backbuf = frontbuf;
  pthread_mutex_lock(&frontbuf_mutex);
  frontbuf = tmp;
  pthread_mutex_unlock(&frontbuf_mutex);
}

static void* poller(void*) {
  for (time_t nextupd = 0; Global::alive();) {
    if (time(nullptr) < nextupd) { usleep(25000); continue; }
    update();
    nextupd = time(nullptr) + 5;
  }
  return nullptr;
}

namespace Clarification {

void fire() {
  Global::fire(poller);
}

void send(int sd) {
  Settings settings;
  send_page(sd, settings);
}

void question(int sd, const string& problem, const string& text) {
  Settings settings;
  time_t now = time(nullptr);
  if (settings.begin <= now && now < settings.end)
    create_question(sd, problem, text, settings);
  else {
    write(sd, "The contest is not running.", 27);
  }
}

} // namespace Clarification
