#ifndef MESSAGE_H
#define MESSAGE_H

#include <sys/types.h>

enum {NOMSG=0,PING,IMALIVE,STOP,RELOAD};
struct Message {
  long mtype;
  struct {
    key_t sender_key;
  } data;
  Message(long mtype = NOMSG, key_t sender_key = 0);
  void send(key_t);
  void process(); // only for main queue
};

class MessageQueue {
  public:
    MessageQueue(); // RAII
    ~MessageQueue(); // RAII
    key_t key() const;
    void update() const; // only for main queue
    Message receive(int timeout = -1) const; // secs. -1 blocks. for tmp queues
  private:
    key_t key_;
    int msqid;
};

#endif
