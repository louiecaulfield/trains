CFLAGS=-Wall -g -lcurl -ltidy -lsqlite3 -I/opt/local/include

all: CFLAGS+=-DNDEBUG
all: executable 

debug: executable 
executable: train_scraper sncf_stations_to_db

train_scraper: html.o curl_tidy.o sncf.o trains.o database.o util.o

clean: 
	rm train_scraper sncf_stations_to_db *.o
