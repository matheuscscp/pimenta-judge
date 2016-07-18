# pimenta-judge
ACM ICPC contest system for small competitions made in C++ to run on GNU/Linux, because [BOCA](https://github.com/cassiopc/boca) is too freaking much!

## Languages currently supported
| Language | Compilation command | Execution command |
| -------- | ------------------- | ----------------- |
| C        | `gcc -std=c11 -lm`  | GNU/Linux default |
| C++      | `g++ -std=c++1y`    | GNU/Linux default |
| Java     | `javac`             | `java -cp`        |
| Python   |                     | `python`          |
| Python 3 |                     | `python3`         |

## Installation
```bash
$ git clone https://github.com/matheuscscp/pimenta-judge.git
$ cd pimenta-judge
$ make
$ sudo make install
```

## Creating, starting and stopping a contest
Choose a directory name, like `mycontest`, an available network port, like 8000, and type the following commands:
```bash
$ pjudge install mycontest
$ cd mycontest
$ pjudge start 8000
```
Then access [http://localhost:8000/](http://localhost:8000/). To stop, type:
```bash
$ pjudge stop
```

### VERY IMPORTANT WARNINGS AND HINTS
* (Warning) `pjudge start` creates a file inside the folder (`mycontest/contest.bin`) to avoid future start commands from lauching the judge again and `pjudge stop` removes this file. So if the computer shuts down instantly (as in an energy cut), you should remove this file to start the judge again!
* (Warning) Except for web user sessions, `pjudge` completely relies on its file structure (described below). Anything you change in the directory will take effect without the need of a restart!
* (Hint) All it takes to run a `pjudge` instance is a directory and a port. So you can have multiple contests running in the same host!
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

The problems' colors are in the beginning of `www/script.js`:
```javascript
var problems = [];
problems["A"] = "#FF0000";
problems["B"] = "#00FF00";
problems["C"] = "#0000FF";
problems["D"] = "#FF6600";
problems["E"] = "#006600";
problems["F"] = "#003399";
problems["G"] = "#FFCC00";
problems["H"] = "#FFFFFF";
problems["I"] = "#000000";
problems["J"] = "#FFFF00";
```
##### Warning
Keep problems sorted by alphabetical order in this file!

#### File `settings.txt`
```
Start:    2015 09 01 19 00
Duration: 300
Freeze:   60
Blind:    15
C:        allowed
C++:      allowed
Java:     allowed
Python:   forbidden
Python3:  forbidden
A(time-limit-per-file-in-seconds): 4
B(the-alphabetical-order-must-be-followed): 3
C(these-comments-are-useless-and-can-be-removed): 5
D: 1
```
* Field `Start:` is a timestamp in the format YYYY MM DD hh mm.
* Fields `Duration:`, `Freeze:` and `Blind:` are time in minutes.
* Field `Duration:` is relative to the beginning of the contest.
* Fields `Freeze:` and `Blind:` are relative to the end of the contest.

##### Warning
Keep problems sorted by alphabetical order in this file!

#### File `teams.txt`
```
"Team 1 Name" team1username team1password
"Team 2 Name" team2username team2password
"Team 3 Name" team3username team3password
```

#### File `clarifications.txt`
```
global A "Problem A question available to all teams" "Answer"
team1username C "Problem C question privately answered to team1username" "Answer"
```

### Optionally added by contest owner
| Type | Name                                  | Function             |
| ---- | ------------------------------------- | -------------------- |
| File | [`statement.pdf`](#file-statementpdf) | Problems' statements |

#### File `statement.pdf`
Contest owner may add `statement.pdf` PDF file containing the problems' statements. It will be available in the web interface!

### Automatically generated during contest execution
| Type      | Name              | Function                                                |
| --------- | ----------------- | ------------------------------------------------------- |
| Directory | `attempts`        | All files related to contestants' attempts              |
| Directory | `questions`       | Clarification requests                                  |
| File      | `attempts.txt`    | Attempts data                                           |
| File      | `nextid.bin`      | Next attempt ID                                         |
