#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <curl/curl.h>
#include <tidy/tidy.h>
#include <tidy/buffio.h>
#include "trains.h"
#include "include/dbg.h"
#include "html.h"
#include "database.h"
#include "curl_tidy.h"
#include "sncf.h"

#define TMP_FN_TEMPLATE "schedrip-XXXXXX"

int main(int argc, char *argv[])
{
	int res, consecutive_success = 0;
	CURL *curl_hdl = NULL;
	sqlite3 *db_hdl = NULL;
 	TidyDoc tdoc = NULL;
	char *link, *new_link;
	struct tm time_dep;
	char *stn_departure, *stn_arrival;
	struct train_t *trains = NULL;
	size_t ntrains;

	//Set up Curl 
	check(curl_tidy_init(&curl_hdl)==0,"Failed to initialise curl");
	debug("curl_hdl %p", curl_hdl);

	//Set up database and get names
	res = database_init(&db_hdl, "test.db");
	check(res==0, "Failed to open database"); 
	res = station_get_name(db_hdl, "Brussels Midi", &stn_departure);
	check(res==0, "Failed to get departure station name");
	res = station_get_name(db_hdl, "Lyon", &stn_arrival);
	check(res==0, "Failed to get arrival station name");

	//Send search query 
	time_dep.tm_sec = time_dep.tm_min = 0;
	time_dep.tm_hour = 15;
	time_dep.tm_mday = 4;
	time_dep.tm_mon = 2-1; //February
	time_dep.tm_year = 2014 - 1900;
	time_dep.tm_isdst = -1;	

	res = sncf_post_form(curl_hdl, &tdoc, 
		&link, &time_dep, 
		stn_departure, stn_arrival);
	check(res==0, "Failed to perform query");
	log_info("Initialized (%d) - link = %s", res, link);

	//Fetch, parse, print
	for(int i = 0; i < 1000; i++) {
		debug("Next link %s", link);
		tidyRelease(tdoc);
		res = curl_tidy_get(curl_hdl, link, &tdoc);	
		check(res ==  0, "failed to fetch results page");

		res = sncf_find_next_results_link(tdoc, &new_link);
		check(res == 0, "failed to get link to next results");

/*
 * An error in the SNCF site results in occasionally being
 * sent to the same results page. This means getting stuck
 * in a loop. If that happens, a workaround is to start a
 * new query and continue from there
 */
		if(!strcmp(link, new_link)) {
			free(link);
			free(new_link);

			log_warn("Next results page is the same as the current one (%d successes)", consecutive_success);
			check(consecutive_success > 2,
				"Only 2 success before loop, aborting");
			localtime_r(&trains[ntrains-1].time_departure, &time_dep);

			consecutive_success = 0;
			tidyRelease(tdoc);
			res = sncf_post_form(curl_hdl, &tdoc, 
				&link, &time_dep, 
				stn_departure, stn_arrival);
			check(res==0, "Failed to perform query");
			continue;
		}

		free_trains(&trains, &ntrains);
		ntrains = sncf_parse_results(tdoc, &trains);
		check(ntrains, "No trains found");
		consecutive_success++; 
		print_trains(trains, ntrains, 0);

		free(link);
		link = new_link;
	}
	free(link);
error:
	//Dump last download (in case of error)
	if(tdoc) tidySaveFile(tdoc, "dumpfile-exit.html");
	tidyRelease(tdoc);
	curl_tidy_cleanup(curl_hdl);
	database_cleanup(db_hdl);
	return 0;
}
