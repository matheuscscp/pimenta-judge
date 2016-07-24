#include <cstring>

#include <unistd.h>
#include <sys/stat.h>

#include "clarification.hpp"

#include "global.hpp"

using namespace std;

static string clarifications(const string& team) {
  struct clarification {
    string str;
    bool read(FILE* fp, const string& team) {
      string tmp;
      for (char c; fscanf(fp, "%c", &c) == 1 && c != ' '; tmp += c);
      if (tmp == "") return false;
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
      if (tmp != "global" && tmp != team) str = "";
      return true;
    }
  };
  
  FILE* fp = fopen("clarifications.txt", "r");
  if (!fp) return "";
  string buf;
  for (clarification c; c.read(fp, team); buf += c.str);
  fclose(fp);
  return buf;
}

namespace Clarification {

void send(const string& team, int sd) {
  string response =
    "HTTP/1.1 200 OK\r\n"
    "Connection: close\r\r"
    "Content-Type: text/html\r\n"
    "\r\n"
    "<table class=\"data\">\n"
      "<tr><th>Problem</th><th>Question</th><th>Answer</th></tr>\n"+
       clarifications(team)+
    "</table>\n"
  ;
  write(sd, response.c_str(), response.size());
}

void question(
  int sd, const string& team, const string& problem, const string& text
) {
  Settings settings;
  
  // check time
  time_t now = time(nullptr);
  if (now < settings.begin || settings.end <= now) {
    write(sd, "The contest is not running.", 27);
    return;
  }
  
  // check problem
  if (
    problem.size() == 0 || problem.size() > 1 ||
    problem[0] < 'A' || problem[0] > char('A' + settings.problems.size() - 1)
  ) {
    write(sd, "Choose a problem!", 17);
    return;
  }
  
  // check text
  if (text.size() == 0) {
    write(sd, "Write something!", 16);
    return;
  }
  
  // save
  mkdir("questions", 0777);
  char fn[17];
  strcpy(fn, "questions/XXXXXX");
  Global::lock_question_file();
  int fd = mkstemp(fn);
  dprintf(fd, "Team: %s\n", team.c_str());
  dprintf(fd, "Problem: %c\n", problem[0]);
  dprintf(fd, "Question: %s\n", text.c_str());
  close(fd);
  Global::unlock_question_file();
}

} // namespace Clarification
