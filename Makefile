CFLAGS=-Wall -g -lcurl -ltidy -I/opt/local/include

all: main

debug: CFLAGS+=-DDEBUG 
debug: main


main: html.o curl_http.o sncf.o

clean: 
	rm main *.o
