# pimenta-judge
ACM ICPC contest system for GNU/Linux.

The currently supported programming languages are:

| Language | Compilation command | Execution command |
| -------- | ------------------- | ----------------- |
| C        | `gcc -std=c11 -lm`  | GNU/Linux default |
| C++      | `g++ -std=c++1y`    | GNU/Linux default |
| Java     | `javac`             | `java -cp`        |
| Python   |                     | `python`          |
| Python 3 |                     | `python3`         |

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
* `pjudge` do *not* supports memory limit configuration. If you need it, feel free to use GNU/Linux commands, like `ulimit -a`, `ulimit -s` and `ulimit -v`, *before* starting `pjudge`!

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
      "day": 7,
      "hour": 0,
      "minute": 0
    },
    "duration": 300,
    "freeze": 60,
    "blind": 15,
    "languages": {
      ".c": {
        "name": "C",
        "flags": "-std=c11 -lm",
        "enabled": true
      },
      ".cpp": {
        "name": "C++",
        "flags": "-std=c++1y",
        "enabled": true
      },
      ".java": {
        "name": "Java",
        "flags": "",
        "enabled": true
      },
      ".py": {
        "name": "Python",
        "flags": "",
        "enabled": false
      },
      ".py3": {
        "name": "Python 3",
        "flags": "",
        "enabled": false
      }
    },
    "problems": [
      {
        "name": "A",
        "timelimit": 4,
        "autojudge": true,
        "color": "#ff0000",
        "enabled": true
      },
      {
        "name": "B",
        "timelimit": 3,
        "autojudge": false,
        "color": "#00ff00",
        "enabled": true
      },
      {
        "name": "C",
        "timelimit": 1,
        "autojudge": true,
        "color": "#0000ff",
        "enabled": false
      },
      {
        "name": "D",
        "timelimit": 1,
        "autojudge": true,
        "color": "#ff6600",
        "enabled": false
      },
      {
        "name": "E",
        "timelimit": 1,
        "autojudge": true,
        "color": "#006600",
        "enabled": false
      },
      {
        "name": "F",
        "timelimit": 1,
        "autojudge": true,
        "color": "#003399",
        "enabled": false
      },
      {
        "name": "G",
        "timelimit": 1,
        "autojudge": true,
        "color": "#ffcc00",
        "enabled": false
      },
      {
        "name": "H",
        "timelimit": 1,
        "autojudge": true,
        "color": "#ffffff",
        "enabled": false
      },
      {
        "name": "I",
        "timelimit": 1,
        "autojudge": true,
        "color": "#000000",
        "enabled": false
      },
      {
        "name": "J",
        "timelimit": 1,
        "autojudge": true,
        "color": "#ffff00",
        "enabled": false
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

##### WARNING
Problems must be A, B, C and so on. No letter can be skipped! And remember to keep the alphabetical order!

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
| File      | `log.txt`       | Logs of important web session events       |
