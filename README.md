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
  * [VERY IMPORTANT WARNINGS AND HINTS](#very-important-warnings-and-hints)
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

### VERY IMPORTANT WARNINGS AND HINTS
* (WARNING) `pjudge start` creates a file inside the folder (`mycontest/contest.bin`) to avoid future start commands from lauching the judge again and `pjudge stop` removes this file. So if the computer shuts down instantly (as in an energy cut), you should remove this file to start the judge again!
* (WARNING) Except for web sessions, `pjudge` completely relies on its file structure (described below). Anything you change in the directory will take effect without the need of a restart!
* (Hint) All it takes to run a `pjudge` instance is a directory and a network port. So you can have multiple contests running in the same host!
* (Hint) `pjudge` do *not* supports memory limit configuration. If you need it, feel free to use GNU/Linux commands, like `ulimit -a`, `ulimit -s` and `ulimit -v`, *before* starting `pjudge`!

## Directory and file structure

### Automatically generated at contest creation
| Type      | Name                                            | Function              |
| --------- | ----------------------------------------------- | --------------------- |
| Directory | [`problems`](#directory-problems)               | Secret test cases     |
| Directory | [`www`](#directory-www)                         | Web interface         |
| File      | [`settings.txt`](#file-settingstxt)             | Time settings         |
| File      | [`teams.txt`](#file-teamstxt)                   | User accounts         |
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
Files of the web interface, like HTML, CSS and JavaScript. Feel free to modify the web interface of your contest! Just remember that some parts of the web stuff are C++ hard-coded. Whatever you don't find in the `www` folder, you will find in the C++ source code!

#### File `settings.txt`
```
Start:    2015 09 01 19 00
Duration: 300
Freeze:   60
Blind:    15
C:        enabled
C++:      enabled
Java:     enabled
Python:   disabled
Python3:  disabled
A:        4 autojudge
B:        3 manual
```
* `Start` follows the format YYYY MM DD hh mm.
* `Duration`, `Freeze` and `Blind` are time in minutes.
* `Duration` is relative to the beginning of the contest.
* `Freeze` and `Blind` are relative to the end of the contest.
* The first setting of a problem (the number) is the time limit in seconds.

##### WARNING
Problems must be A, B, C and so on. No letter can be skipped! And remember to keep the alphabetical order!

#### File `teams.txt`
```
"Team 1" team1username team1password
"Team 2" team2username team2password
"Team 3" team3username team3password
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
| Type      | Name                                | Function                                   |
| --------- | ----------------------------------- | ------------------------------------------ |
| Directory | `attempts`                          | All files related to contestants' attempts |
| Directory | `questions`                         | Clarification requests                     |
| File      | [`attempts.txt`](#file-attemptstxt) | Attempts data                              |
| File      | `nextid.bin`                        | Next attempt ID                            |
| File      | `log.txt`                           | Logs of important web session events       |

#### File `attempts.txt`
Each line in this file stands for one attempt and follows the format:
```
ID,Problem,Verdict,Time,Runtime,Username,IP,Fullname,Status
```
where:
* `ID` is a unique integer to identify the attempt.
* `Problem` is a capital letter to identify the problem of the attempt.
* `Verdict` may be:
  * `AC` — Accepted. The only "yes" answer.
  * `CE` — Compile Error.
  * `RTE` — Runtime Error.
  * `TLE` — Time Limit Exceeded.
  * `WA` — Wrong Answer.
  * `PE` — Presentation Error.
* `Time` is the number of minutes passed since the beginning of the contest when the attempt was made.
* `Runtime` is the maximum time in seconds that the attempt took to execute.
* `Username` is the user name.
* `IP` is the IP address that sent the attempt.
* `Fullname` is the full name of the user.
* `Status` may be `judged` or `tojudge`. Only `judged` attempts will affect the scoreboard.

##### WARNING
Problems with the `autojudge` setting will produce `judged` attempts, while problems with the `manual` setting will produce `tojudge` attempts.
