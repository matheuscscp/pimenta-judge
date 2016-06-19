# pimenta-judge
ACM ICPC contest system for training sessions.

## Installation
```bash
$ git clone https://github.com/matheuscscp/pimenta-judge.git
$ cd pimenta-judge
$ make
$ sudo make install
```

## Creating and starting a contest
Choose an available port and:
```bash
$ pjudge install contest_name
$ cd contest_name
$ pjudge start <port>
```
Then access [http://localhost:\<port\>/](http://localhost:\<port\>/).

## Stopping a contest
```bash
$ cd contest_name
$ pjudge stop
```
