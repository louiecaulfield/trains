#ifndef SNCF_H
#define SNCF_H
#include <time.h>

int construct_postfields(CURL *curl_hdl, char ** postfields);

int sncf_find_next_results(TidyDoc tdoc, TidyNode summary, char ** link);
int sncf_parse_pricesummary(TidyDoc tdoc);

struct train_info {
	char * stn_departure;
	char * stn_arrival;
	time_t time_depature;
	time_t time_arrival;
	char * operator;	
	unsigned int price;
};

#endif
