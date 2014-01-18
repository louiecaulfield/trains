CFLAGS=-Wall -g -lcurl -ltidy -lsqlite3 -I/opt/local/include

all: train_scraper db

debug: CFLAGS+=-DDEBUG 
debug: all 


train_scraper: html.o curl_tidy.o sncf.o trains.o database.o util.o
db: sncf_stations_to_db

clean: 
	rm train_scraper sncf_stations_to_db *.o
