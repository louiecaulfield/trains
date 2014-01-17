CFLAGS=-Wall -g -lcurl -ltidy -lsqlite3 -I/opt/local/include

all: main db

debug: CFLAGS+=-DDEBUG 
debug: all 


main: html.o curl_tidy.o sncf.o trains.o database.o util.o
db: sncf_stations_to_db

clean: 
	rm main sncf_stations_to_db *.o
