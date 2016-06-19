# pimenta-judge
ACM ICPC contest system for training sessions made with C++ 11 to run on GNU/Linux.

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
Then access [http://localhost:8000/](http://localhost:8000/).

To stop, type:
```bash
$ pjudge stop
```

### VERY IMPORTANT HINTS
* `pjudge start` creates a binary file inside the folder (`mycontest/contest.bin`) to avoid future start commands from lauching the judge again and `pjudge stop` removes this file. So if the computer shuts down instantly (as in an energy cut), you should remove this file to start the judge again!
* `pjudge` keeps nothing of its file structure (described below) in memory. Anything you change in the directory will take effect instantly!
* All it takes to run a `pjudge` instance is a directory and a port. So you can have multiple contests running in the same host!

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

Example: `A.in1`, `A.in2`, `A.sol1`, `A.sol2`, `B.in1`, `B.in2`, `B.sol1` and `B.sol2`

##### Warning
For each problem, the numbers must start with 1 and follow the natural order. For example, the following files will not work:

`A.in1`, `A.in2`, `A.sol3` and `A.sol3`

#### Directory `www`
Files of the web interface, like HTML, CSS and JavaScript. Feel free to modify the web interface of your contest! Just remember that some things are C++ hard-coded (like [this](https://github.com/matheuscscp/pimenta-judge/blob/master/src/scoreboard.cpp), [this](https://github.com/matheuscscp/pimenta-judge/blob/master/src/clarification.cpp) and [this](https://github.com/matheuscscp/pimenta-judge/blob/master/src/webserver.cpp)) and the problems' colors are in the beginning of `www/script.js`:
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
Keep the alphabetical order when setting the colors inside the JavaScript source!

#### File `settings.txt`
```
Begin:  2015 09 01 19 00
End:    2015 12 25 23 50
Freeze: 2015 12 25 23 50
Blind:  2015 12 25 23 50
A(time-limit-per-file-in-seconds): 4
B(these-comments-are-useless-and-can-be-removed): 3
C(so-the-alphabetical-order-must-be-followed): 5
D: 1
```

#### File `teams.txt`
```
"Team 1 Name" team1username team1password
"Team 2 Name" team2username team2password
"Team 3 Name" team3username team3password
```

#### File `clarifications.txt`
tmp
