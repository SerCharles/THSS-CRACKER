server: server.o utils.o
	gcc -Wall -D_FILE_OFFSET_BITS=64 server.o utils.o -o server
server.o: server.c utils.h
	gcc -Wall -D_FILE_OFFSET_BITS=64 -c server.c -o server.o
utils.o: utils.c utils.h
	gcc -Wall -D_FILE_OFFSET_BITS=64 -c utils.c -o utils.o
clean:
	rm server server.o utils.o
