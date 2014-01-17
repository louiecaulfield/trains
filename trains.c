#include <stdlib.h>
#include <stdio.h>
#include "trains.h"

void print_trains(struct train_list_t *trains, int header)
{
	struct train_list_t *train;	
	char format[255]= "%15s | %15s | %20s | %20s | %8s | %20s\n";
	if(header) {
		printf(format, "Departure", "Arrival", "TOD", "arr", "Price", "Operator");
		printf("-------------------------\n");
	}
	for(train = trains; train; train = train->next)
	{
		char stn_dep[10], stn_arr[10], time_arr[20], time_dep[20], price[10];
		struct tm tm;	
		localtime_r(&train->train.time_departure, &tm);
		strftime(time_dep, 20, "%v %R", &tm);
		localtime_r(&train->train.time_arrival, &tm);
		strftime(time_arr, 20, "%v %R", &tm);
		sprintf(price, "%2.2f", train->train.price);	

		snprintf(stn_dep, 10, "%d", train->train.stn_departure);
		snprintf(stn_arr, 10, "%d", train->train.stn_arrival);

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
