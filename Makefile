CFLAGS=-Wall -g
LDFLAGS=-lcurl -ltidy -lsqlite3

OBJECTS=html.o curl_tidy.o sncf.o trains.o database.o util.o train_scraper.o
EXECUTABLE=train_scraper

all: CFLAGS+=-DNDEBUG
all: $(EXECUTABLE) 

debug: $(EXECUTABLE) 

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean: 
	rm train_scraper *.o
