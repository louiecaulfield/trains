#ifndef SNCF_H
#define SNCF_H
#include <time.h>
#include <curl/curl.h>
#include <tidy/tidy.h>
#include "trains.h"

int construct_postfields(CURL *curl_hdl, char ** postfields, 
	struct tm *time_departure, 
	const char *stn_departure, const char *stn_arrival);

int sncf_post_form(CURL *curl_hdl, TidyDoc *tdoc, char ** link, 
	struct tm *time_departure, 
	char *stn_departure, char *stn_arrival);
int sncf_find_next_results_link(TidyDoc tdoc, char ** link);
size_t sncf_parse_results(sqlite3 *db_hdl, TidyDoc tdoc, struct train_t **ret);

#endif
