#include <unistd.h>
#include <sys/msg.h>

#include "rejudger.hpp"

#include "global.hpp"

static int msqid;

static void* rejudger(void*) {
  rejudgemsg msg(0, 0);
  while (Global::alive()) {
    if (msgrcv(msqid, &msg, msg.size(), 0, IPC_NOWAIT) < 0) {
      usleep(25000); continue;
    }
    Rejudger::rejudge(msg.id, msg.verdict);
  }
  return nullptr;
}

namespace Rejudger {

void fire(int msq) {
  msqid = msq;
  //Global::fire(rejudger); FIXME
}

void rejudge(int id, char verdict) {
  Global::lock_att_file();
  FILE* fp = fopen("attempts.bin", "r+b");
  if (fp) {
    Attempt att;
    while (fread(&att, sizeof att, 1, fp) == 1) {
      if (att.id == id) {
        att.verdict = verdict;
        fseek(fp, -(sizeof att), SEEK_CUR);
        fwrite(&att, sizeof att, 1, fp);
        break;
      }
    }
    fclose(fp);
  }
  Global::unlock_att_file();
}

} // namespace Rejudger
