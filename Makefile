all:
	g++ -std=c++1y src/*.cpp -o pjudge -pthread

install:
	cp pjudge /usr/local/bin
