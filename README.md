# pimenta-judge
ACM ICPC contest system for GNU/Linux.

## Table of contents
* [Installation](#installation)
* [Usage](#usage)
  * [Hints](#hints)
* [Directory and file structure](#directory-and-file-structure)
  * [Automatically generated at contest creation](#automatically-generated-at-contest-creation)
  * [Optionally added by contest owner](#optionally-added-by-contest-owner)
  * [Automatically generated during contest execution](#automatically-generated-during-contest-execution)

## Installation
```bash
$ git clone https://github.com/matheuscscp/pimenta-judge.git
$ cd pimenta-judge
$ make
$ sudo make install
```

## Usage
To create a contest, choose a directory name, like `mycontest`, and type:
```bash
$ pjudge install mycontest
```
To start the system, type:
```bash
$ cd mycontest
$ pjudge start
```
Then access [http://localhost:8000/](http://localhost:8000/). To stop, type:
```bash
$ pjudge stop
```

### Hints
* All it takes to run a `pjudge` instance is a directory and a network port. So you can have multiple contests running in the same host!

## Directory and file structure

### Automatically generated at contest creation
| Type      | Name                                            | Function              |
| --------- | ----------------------------------------------- | --------------------- |
| Directory | [`problems`](#directory-problems)               | Secret test cases     |
| Directory | [`www`](#directory-www)                         | Web interface         |
| File      | [`settings.json`](#file-settingsjson)           | General settings      |
| File      | [`clarifications.txt`](#file-clarificationstxt) | Clarification answers |

#### Directory `problems`
```
problems/
├── A
│   ├── input
│   │   ├── file1_huge_case
│   │   └── file2
│   └── output
│       ├── file1_huge_case
│       └── file2
└── B
    ├── input
    │   └── file1
    └── output
        └── file1
```

#### Directory `www`
Files of the web interface, like HTML, CSS and JavaScript. Feel free to modify the web interface of your contest!

#### File `settings.json`
```json
{
  "webserver": {
    "port": 8000,
    "client_threads": 4,
    "request": {
      "timeout": 5,
      "max_uri_size": 1024,
      "max_header_size": 1024,
      "max_headers": 1024,
      "max_payload_size": 1048576
    },
    "session": {
      "clean_period": 86400
    }
  },
  "contest": {
    "start": {
      "year": 2016,
      "month": 8,
      "day": 9,
      "hour": 20,
      "minute": 0
    },
    "duration": 300,
    "freeze": 60,
    "blind": 15,
    "languages": {
      ".c": {
        "name": "C",
        "compile": "gcc -std=c11 %s -o %p/%P -lm",
        "run": "%p/%P",
        "flags": "-std=c11 -lm",
        "enabled": true
      },
      ".cpp": {
        "name": "C++",
        "compile": "g++ -std=c++1y %s -o %p/%P",
        "run": "%p/%P",
        "flags": "-std=c++1y",
        "enabled": true
      },
      ".java": {
        "name": "Java",
        "compile": "javac %s",
        "run": "java -cp %p %P",
        "flags": "",
        "enabled": true
      }
    },
    "problems": [
      {
        "dirname": "A",
        "timelimit": 4,
        "memlimit": 100000,
        "autojudge": true,
        "color": "#ff0000",
        "enabled": true
      },
      {
        "dirname": "B",
        "timelimit": 3,
        "autojudge": true,
        "color": "#00ff00",
        "enabled": true
      }
    ]
  },
  "users": {
    "team1username": {
      "fullname": "Team 1 Name",
      "password": "team1password"
    },
    "team2username": {
      "fullname": "Team 2 Name",
      "password": "team2password"
    },
    "team3username": {
      "fullname": "Team 3 Name",
      "password": "team3password"
    }
  }
}
```

#### File `clarifications.txt`
```
global A "Problem A question available to all teams" "Answer"
team1username B "Problem B question privately answered to Team 1" "Answer"
```

### Optionally added by contest owner
| Type | Name                                  | Function             |
| ---- | ------------------------------------- | -------------------- |
| File | [`statement.pdf`](#file-statementpdf) | Problems' statements |

#### File `statement.pdf`
Contest owner may add `statement.pdf` PDF file containing the problems' statements. It will be available in the web interface!

### Automatically generated during contest execution
| Type      | Name            | Function                                   |
| --------- | --------------- | ------------------------------------------ |
| Directory | `attempts`      | All files related to contestants' attempts |
| Directory | `questions`     | Clarification requests                     |
| File      | `contest.bin`   | System usage (don't touch)                 |
| File      | `attempts.json` | Attempts data                              |
| File      | `log.txt`       | Log of web session events                  |
