CC=gcc -std=c89 -Wpedantic


all:  clear nave porto master

master:
		clear
		mkdir -p bin
		$(CC) ./src/master.c  -o bin/master.out -lm -pthread
		./bin/master.out > out.txt


nave:
	clear
	mkdir -p bin
	$(CC) ./src/lotto.c ./src/nave.c  -o bin/nave.out -lm -pthread




porto:
	clear
	mkdir -p bin
	$(CC) ./src/lotto.c ./src/porto.c  -o bin/porto.out -lm -pthread
	

clear:
	rm -rf bin
	rm -rf out.txt
	clear

