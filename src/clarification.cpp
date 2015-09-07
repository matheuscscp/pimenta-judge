#include <cstring>

#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>

#include "clarification.h"

#include "global.h"

using namespace std;

static string buf1, buf2;
static string* frontbuf = &buf1;
static string* backbuf = &buf2;
static pthread_mutex_t frontbuf_mutex = PTHREAD_MUTEX_INITIALIZER;

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
    "<html>\n"
    "  <head>\n"
    "    <script>\n"
    "      function sendform() {\n"
    "        prob = document.getElementById(\"problem\");\n"
    "        if (prob.value == \"\") {\n"
    "          response = document.getElementById(\"response\");\n"
    "          response.innerHTML = \"Choose a problem!\";\n"
    "          return;\n"
    "        }\n"
    "        ques = document.getElementById(\"question\");\n"
    "        if (ques.value == \"\") {\n"
    "          response = document.getElementById(\"response\");\n"
    "          response.innerHTML = \"Write something!\";\n"
    "          return;\n"
    "        }\n"
    "        response = document.getElementById(\"response\");\n"
    "        response.innerHTML = \"Question sent. Wait and refresh.\";\n"
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
    "        xmlhttp.setRequestHeader(\"Problem\", prob.value);\n"
    "        xmlhttp.setRequestHeader(\"Question\", ques.value);\n"
    "        xmlhttp.send();\n"
    "        prob.value = \"\";\n"
    "        ques.value = \"\";\n"
    "      }\n"
    "    </script>\n"
    "  </head>\n"
    "  <body>\n"
    "    <h1 align=\"center\">Pimenta Judgezzz~*~*</h1>\n"
    "    <h2 align=\"center\">Clarifications</h2>\n"
    "    <h4 align=\"center\" id=\"response\"></h4>\n"
    "    <table align=\"center\">\n"
    "      <tr>\n"
    "        <td>Problem:</td>\n"
    "        <td>"+select+"</td>\n"
    "      </tr>\n"
    "      <tr>\n"
    "        <td>Question:</td>\n"
    "        <td><textarea"
    "         id=\"question\""
    "         rows=\"4\" cols=\"50\""
    "        ></textarea></td>\n"
    "      </tr>\n"
    "      <tr><td><button onclick=\"sendform()\">Send</button></td></tr>\n"
    "    </table>\n"
    "    <table align=\"center\" border=\"3\">\n"
    "      <tr><th>Problem</th><th>Question</th><th>Answer</th></tr>\n"+
           clarifications+
    "    </table>\n"
    "  </body>\n"
    "</html>\n"
  ;
  write(sd, response.c_str(), response.size());
}

static void create_question(int sd, char* buf) {
  // assemble question
  char* problem = strstr(buf, "Problem: ")+9;
  if (strlen(problem) == 0) {
    write(sd, "Choose a problem!", 17);
    return;
  }
  char* question = strstr(buf, "Question: ")+10;
  while (question[0] != '\r') question++;
  question[0] = 0;
  question = strstr(buf, "Question: ")+10;
  if (strlen(question) == 0) {
    write(sd, "Write something!", 16);
    return;
  }
  write(sd, "Question sent. Wait and refresh.", 32);
  
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
  close(fd);
  remove(fn);
  FILE* fp = fopen(fn, "w");
  fprintf(fp, "Problem: %c\nQuestion: %s\n", problem[0], question);
  fclose(fp);
  Global::unlock_question_file();
  system("gedit %s &", fn);
}

static void* client(void* ptr) {
  // fetch socket
  int* sdptr = (int*)ptr;
  int sd = *sdptr;
  delete sdptr;
  
  char* buf = new char[1 << 10];
  
  // check contest time
  Settings settings;
  if (time(nullptr) < settings.begin) {
    delete[] buf;
    close(sd);
    return nullptr;
  }
  
       if (!read_headers(sd, buf))        write(sd, "Incomplete request!", 19);
  else if (buf[0] == 'G')                 send_page(sd, settings);
  else if (time(nullptr) < settings.end)  create_question(sd, buf);
  else                                    write(sd, "Contest is over!", 16);
  
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
  addr.sin_port = htons(to<uint16_t>(Global::arg[5]));
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

static void update() {
  //TODO
  
  // update back buffer
  (*backbuf) = "<tr><td>A</td><td>vei</td><td>kkk</td></tr>";
  
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
  Global::fire(server);
  Global::fire(poller);
}

} // namespace Clarification
