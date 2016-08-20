#ifndef MESSAGE_H
#define MESSAGE_H

#include <sys/types.h>

enum {NOMSG=0,PING,IMALIVE,STOP,RERUN_ATT};
struct Message {
  long mtype;
  union {
    key_t sender_key;
    int attid;
  } data;
  Message(long mtype = NOMSG);
  void send(key_t);
  void process(); // only for main queue
};

struct PingMessage : public Message {
  PingMessage(key_t sender_key);
};

struct RerunAttMessage : public Message {
  RerunAttMessage(int attid);
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
