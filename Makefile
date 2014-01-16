CFLAGS=-Wall -g -lcurl -ltidy -lsqlite3 -I/opt/local/include

all: main db

debug: CFLAGS+=-DDEBUG 
debug: all 


main: html.o curl_http.o sncf.o
db: sncf_stations_to_db

clean: 
	rm main sncf_stations_to_db *.o
