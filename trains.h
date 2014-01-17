#ifndef TRAINS_H
#define TRAINS_H
#include <time.h>

struct train_t {
	char * stn_departure;
	char * stn_arrival;
	time_t time_departure;
	time_t time_arrival;
	char * operator;	
	float price;
};

void print_trains(struct train_t *trains, size_t n, int header);
void free_trains(struct train_t **trains, size_t *ntrains);

#endif
