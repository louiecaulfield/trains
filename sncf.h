#ifndef SNCF_H
#define SNCF_H
#include <time.h>

struct train_info {
	char * stn_departure;
	char * stn_arrival;
	time_t time_departure;
	time_t time_arrival;
	char * operator;	
	float price;
};


int construct_postfields(CURL *curl_hdl, char ** postfields);

int sncf_post_form(CURL *curl_hdl, TidyDoc *tdoc, char ** link);
int sncf_find_next_results(TidyDoc tdoc, char ** link);
size_t sncf_parse_train_info(TidyDoc tdoc, struct train_info **ret);
void sncf_print_train_info(struct train_info *trains, size_t n, int header);
void sncf_free_train_info(struct train_info **trains, size_t *ntrains);

#endif
