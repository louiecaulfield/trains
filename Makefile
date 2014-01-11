CFLAGS=-Wall -lcurl -ltidy -I/opt/local/include

all: main

main: html.o curl_http.o

clean: 
	rm main *.o
