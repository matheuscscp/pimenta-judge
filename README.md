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
Choose an available port, like 8000, and:
```bash
$ pjudge install contest_name
$ cd contest_name
$ pjudge start 8000
```
Then access [http://localhost:8000/](http://localhost:8000/).

## Stopping a contest
```bash
$ cd contest_name
$ pjudge stop
```

## Directory and file structure
When you enter `pjudge install <dirname>`, `pjudge` will create the directory with the following itens:

| Type      | Name                 | Function              |
| --------- | -------------------- | --------------------- |
| Directory | [`problems`]           | Secret test cases     |
| Directory | `www`                | Web interface         |
| File      | `settings.txt`       | Time settings         |
| File      | `teams.txt`          | User accounts         |
| File      | `clarifications.txt` | Clarification answers |

### Directory `problems`
