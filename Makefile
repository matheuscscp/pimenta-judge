all:
	g++ -std=c++1y src/*.cpp -o pjudge -pthread

install:
	cp pjudge /usr/local/bin
	rm -rf /usr/local/share/pjudge
	mkdir /usr/local/share/pjudge
	cp -rf www /usr/local/share/pjudge/www
