chat: server client

install:
	cp src/client bin/
	cp src/server bin/

server:
	gcc -Wall -l pthread -o src/server src/server.c src/CMTP.c src/gestoreUtenti.c src/circBufferUtil.c src/gestoreLogFile.c

client:
	gcc -Wall -l pthread -o src/client src/client.c src/CMTP.c

.PHONY: clean

clean:
	-rm bin/*
