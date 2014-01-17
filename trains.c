#include <stdlib.h>
#include <stdio.h>
#include "trains.h"

void print_trains(struct train_t *trains, size_t n, int header)
{
	int i;
	char format[255]= "%15s | %15s | %20s | %20s | %8s | %20s\n";
	if(header) {
		printf(format, "Departure", "Arrival", "TOD", "arr", "Price", "Operator");
		printf("-------------------------\n");
	}
	for(i = 0; i < n; i++)
	{
		char stn_dep[10], stn_arr[10], time_arr[20], time_dep[20], price[10];
		struct tm tm;	
		localtime_r(&trains[i].time_departure, &tm);
		strftime(time_dep, 20, "%v %R", &tm);
		localtime_r(&trains[i].time_arrival, &tm);
		strftime(time_arr, 20, "%v %R", &tm);
		sprintf(price, "%2.2f", trains[i].price);	

		snprintf(stn_dep, 10, "%d", trains[i].stn_departure);
		snprintf(stn_arr, 10, "%d", trains[i].stn_arrival);

		printf(format,
			stn_dep,	
			stn_arr,	
			time_dep,
			time_arr,
			price,
			trains[i].operator
			);
	}	
}


void free_trains(struct train_t **trains, size_t *ntrains)
{
	size_t i;
	if(*trains) {
		for(i = 0; i < *ntrains; i++) {
			free((*trains)[i].operator);
		}
	}
	free(*trains);
	*trains = NULL;
	*ntrains = 0;
}
