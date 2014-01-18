#ifndef TRAINS_H
#define TRAINS_H
#include <time.h>
#include <sqlite3.h>

struct train_t {
	int stn_departure;
	int stn_arrival;
	time_t time_departure;
	time_t time_arrival;
	char * operator;	
	float price;
	int number;
};

struct train_list_t {
	struct train_t train;
	struct train_list_t *next;
};

void print_trains(sqlite3 *db_hdl, struct train_list_t *trains, int header);
void free_trains(struct train_list_t *trains);
struct train_list_t *get_last_train(struct train_list_t *trains);
#endif
