# pimenta-judge
ACM ICPC contest system for training sessions.

## Installation
```bash
$ git clone https://github.com/matheuscscp/pimenta-judge.git
$ cd pimenta-judge
$ make
$ sudo make install
```

## Creating, starting and stopping a contest
Choose a directory name, like `mycontest`, an available port, like 8000, and type the following commands:
```bash
$ pjudge install mycontest
$ cd mycontest
$ pjudge start 8000
```
Then access [http://localhost:8000/](http://localhost:8000/).

To stop, type:
```bash
$ pjudge stop
```

### Important warning
`pjudge start` creates a file (`contest.bin`) to avoid future start commands from lauching the judge again and `pjudge stop` removes this file. So if the computer shuts down instantly (as in an energy cut), you should remove this file to start the judge again.

## Directory and file structure
When you enter `pjudge install <dirname>`, `pjudge` will create the directory with the following itens:

| Type      | Name                                            | Function              |
| --------- | ----------------------------------------------- | --------------------- |
| Directory | [`problems`](#directory-problems)               | Secret test cases     |
| Directory | [`www`](#directory-www)                         | Web interface         |
| File      | [`settings.txt`](#file-settingstxt)             | Time settings         |
| File      | [`teams.txt`](#file-teamstxt)                   | User accounts         |
| File      | [`clarifications.txt`](#file-clarificationstxt) | Clarification answers |

`pjudge` keeps nothing of these files in memory. Anything you change in the directory will have effect instantly.

### Directory `problems`
tmp

### Directory `www`
tmp

### File `settings.txt`
tmp

### File `teams.txt`
tmp

### File `clarifications.txt`
tmp
