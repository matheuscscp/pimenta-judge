# pimenta-judge
ACM ICPC contest system for small competitions made in C++ to run on GNU/Linux, because [BOCA](https://github.com/cassiopc/boca) is too freaking much!

## Languages currently supported
| Language | Compilation command | Execution command |
| -------- | ------------------- | ----------------- |
| C        | `gcc -std=c11`      | GNU/Linux default |
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

### VERY IMPORTANT WARNINGS AND HINT
* (Warning) `pjudge start` creates a file inside the folder (`mycontest/contest.bin`) to avoid future start commands from lauching the judge again and `pjudge stop` removes this file. So if the computer shuts down instantly (as in an energy cut), you should remove this file to start the judge again!
* (Warning) `pjudge` keeps nothing of its file structure (described below) in memory. Anything you change in the directory will take effect instantly!
* (Hint) All it takes to run a `pjudge` instance is a directory and a port. So you can have multiple contests running in the same host!

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
Just put all of your secret test cases' input and correct output files in this folder, together. No folder separation is needed (or even allowed!). The file names must be in the following format:
* Input file: `<upper_case_letter>.in<positive_number>`
* Correct output file: `<upper_case_letter>.sol<positive_number>`

Example: `A.in1`, `A.in2`, `A.sol1`, `A.sol2`, `B.in1`, `B.in2`, `B.sol1`, `B.sol2`

##### Warning
For each problem, numbers must start with 1 and follow the natural order.

The following files *won't* work: `A.in1`, `A.in2`, `A.sol3`, `A.sol3`

#### Directory `www`
Files of the web interface, like HTML, CSS and JavaScript. Feel free to modify the web interface of your contest! Just remember that some things are C++ hard-coded ([this](https://github.com/matheuscscp/pimenta-judge/blob/master/src/scoreboard.cpp), [this](https://github.com/matheuscscp/pimenta-judge/blob/master/src/clarification.cpp) and [this](https://github.com/matheuscscp/pimenta-judge/blob/master/src/webserver.cpp)) and the problems' colors are in the beginning of `www/script.js`:
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
Keep the alphabetical order when setting the colors!

#### File `settings.txt`
```
Begin:  2015 09 01 19 00
End:    2015 12 25 23 50
Freeze: 2015 12 25 23 50
Blind:  2015 12 25 23 50
A(time-limit-per-file-in-seconds): 4
B(the-alphabetical-order-must-be-followed): 3
C(these-comments-are-useless-and-can-be-removed): 5
D: 1
```

##### Warning
Keep the alphabetical order when setting the time limits!

#### File `teams.txt`
```
"Team 1 Name" team1username team1password
"Team 2 Name" team2username team2password
"Team 3 Name" team3username team3password
```

#### File `clarifications.txt`
```
global A "Question available to all teams" "Answer"
team1username C "Question privately answered to team1username" "Answer"
```

### Optionally added by contest owner
| Type | Name                                  | Function             |
| ---- | ------------------------------------- | -------------------- |
| File | [`statement.pdf`](#file-statementpdf) | Problems' statements |

#### File `statement.pdf`
Contest owner may add `statement.pdf` PDF file containing the problems' statements. It will be available in the web interface!

### Automatically generated during contest execution
| Type      | Name              | Function                                            |
| --------- | ----------------- | --------------------------------------------------- |
| Directory | `attempts`        | Contestants' attempts files                         |
| Directory | `questions`       | Clarification requests                              |
| File      | `ip_by_login.txt` | Log file of all logins with username and IP address |
| File      | `attempts.bin`    | Attempts index                                      |
| File      | `next.bin`        | Next attempt ID                                     |
