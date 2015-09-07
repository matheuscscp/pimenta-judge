#include <cstring>

#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "statement.h"

#include "global.h"

using namespace std;

static void* client(void* ptr) {
  // fetch socket
  int* sdptr = (int*)ptr;
  int sd = *sdptr;
  delete sdptr;
  
  // ignore data
  char* buf = new char[1 << 20];
  while (read(sd, buf, 1 << 20) == (1 << 20));
  
  // check contest time
  Settings settings;
  if (time(nullptr) < settings.begin) {
    write(sd, "Wait for the contest to start!", 30);
    delete[] buf;
    close(sd);
    return nullptr;
  }
  
  // respond
  string response =
    "HTTP/1.1 200 OK\r\n"
    "Connection: close\r\r"
    "Content-Type: application/octet-stream\r\n"
    "Content-Disposition: attachment; filename=\"contest.pdf\"\r\n"
    "\r\n"
  ;
  write(sd, response.c_str(), response.size());
  FILE* fp = fopen("contest.pdf", "rb");
  if (fp) {
    for (size_t rd = 0; (rd = fread(buf, 1, 1 << 20, fp)) > 0;) {
      write(sd, buf, rd);
    }
  }
  fclose(fp);
  
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

namespace Statement {

void fire() {
  Global::fire(server);
}

} // namespace Statement
