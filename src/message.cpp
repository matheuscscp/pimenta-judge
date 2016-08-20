#include <ctime>

#include <unistd.h>
#include <sys/msg.h>

#include "message.hpp"

#include "global.hpp"
#include "judge.hpp"

Message::Message(long mtype) : mtype(mtype) {
  
}

PingMessage::PingMessage(key_t sender_key) : Message(PING) {
  data.sender_key = sender_key;
}

RerunAttMessage::RerunAttMessage(int attid) : Message(RERUN_ATT) {
  data.attid = attid;
}

void Message::send(key_t key) {
  msgsnd(msgget(key,0),this,sizeof data,0);
}

void Message::process() {
  switch (mtype) {
    case PING:
      Message(IMALIVE).send(data.sender_key);
      break;
    case STOP:
      Global::shutdown();
      break;
    case RERUN_ATT:
      Judge::push(data.attid);
      break;
  }
}

MessageQueue::MessageQueue() {
  key_ = 0x713e24a;
  while ((msqid = msgget(key_, (IPC_CREAT|IPC_EXCL)|0777)) < 0) key_++;
}

MessageQueue::~MessageQueue() {
  msgctl(msqid, IPC_RMID, nullptr);
}

key_t MessageQueue::key() const {
  return key_;
}

void MessageQueue::update() const {
  Message msg;
  while (msgrcv(msqid,&msg,sizeof msg.data,0,IPC_NOWAIT) >= 0) msg.process();
}

Message MessageQueue::receive(int timeout) const {
  Message msg;
  if (timeout == -1) { // just block
    msgrcv(msqid,&msg,sizeof msg.data,0,0);
    return msg;
  }
  time_t end = time(nullptr)+timeout, now;
  while (
    (now = time(nullptr)) < end &&
    msgrcv(msqid,&msg,sizeof msg.data,0,IPC_NOWAIT) < 0
  ) {
    usleep(25000);
  }
  if (end <= now) msg.mtype = NOMSG;
  return msg;
}
