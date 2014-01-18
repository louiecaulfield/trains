#include <stdlib.h>
#include <stdio.h>
#include "database.h"
#include "trains.h"

void print_trains(sqlite3 *db_hdl, struct train_list_t *trains, int header)
{
	struct train_list_t *train;	
	char format[255]= "%20s | %20s | %20s | %20s | %8s | %20s\n";
	if(header) {
		printf(format, "Departure", "Arrival", "TOD", "arr", "Price", "Operator");
		printf("-------------------------\n");
	}
	for(train = trains; train; train = train->next)
	{
		char *stn_dep, *stn_arr, time_arr[20], time_dep[20], price[10];
		struct tm tm;	
		localtime_r(&train->train.time_departure, &tm);
		strftime(time_dep, 20, "%e-%b-%Y %R", &tm);
		localtime_r(&train->train.time_arrival, &tm);
		strftime(time_arr, 20, "%e-%b-%Y %R", &tm);
		sprintf(price, "%2.2f", train->train.price);	

		station_get(db_hdl, train->train.stn_departure, &stn_dep);
		station_get(db_hdl, train->train.stn_arrival, &stn_arr);

		printf(format,
			stn_dep,	
			stn_arr,	
			time_dep,
			time_arr,
			price,
			train->train.operator
			);
	}	
}


void free_trains(struct train_list_t *trains)
{
	struct train_list_t *train;	
	for(train = trains; train; train = train->next) {
		free(train->train.operator);
		free(train);
	}
}

struct train_list_t *get_last_train(struct train_list_t *trains)
{
	while(trains->next)
		trains = trains->next;
	return trains;
}
