#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <curl/curl.h>
#include <tidy/tidy.h>
#include <tidy/buffio.h>
#include <unistd.h>
#include "trains.h"
#include "include/dbg.h"
#include "html.h"
#include "database.h"
#include "curl_tidy.h"
#include "sncf.h"

int main(int argc, char *argv[])
{
	errno = 0;
	//CL args
	const char *dbfile = NULL, *stn_departure = NULL, *stn_arrival = NULL;
	int ch,res, consecutive_success = 0;

	//Handles
	CURL *curl_hdl = NULL;
	sqlite3 *db_hdl = NULL;
 	TidyDoc tdoc = NULL;

	//Query initialisers
	char *link, *new_link;
	struct tm time_dep;

	//Parse result holders
	struct train_list_t *trains = NULL;
	size_t n, ntrains;

	//Output
	char str_time_dep[20];
	size_t total = 0;

	//Parse cmdline
	while( (ch = getopt(argc, argv, "d:f:t:")) != -1 ) {
		debug("ch = %d", ch);
		switch(ch) {
		case 'd':
			debug("d %s", optarg);
			dbfile = optarg;
			break;
		case 'f':
			debug("f %s", optarg);
			stn_departure = optarg;
			break;
		case 't':
			debug("t %s", optarg);
			stn_arrival = optarg;
			break;
		case '?':
			if(optopt=='d' || optopt=='f' || optopt=='t') {
				log_info("Missing argument for option -%c", optopt);
				goto usage;
			} else if(isprint(optopt)) {
				log_info("Unknown option '-%c'", optopt);
			} else {
				log_info("Unknown option character '\\x%x'", optopt);
			}
			break;	
		default:
			debug("err got c=%d (opterr=%d, optopt=%c, optind=%d, optarg=%s)", ch, opterr, optopt, optind, optarg);
			goto usage;	
		}
	}
	if(!dbfile || !stn_departure || !stn_arrival) goto usage;
	debug("Starting %s with dbfile='%s', stn_dep='%s', stn_arr='%s'",
		argv[0], dbfile, stn_departure, stn_arrival);

	//Set up Curl 
	check(curl_tidy_init(&curl_hdl)==0,"Failed to initialise curl");
	debug("curl_hdl %p", curl_hdl);

	//Set up database and get names
	res = database_init(&db_hdl, dbfile);
	check(res==0, "Failed to open database"); 

	//Send search query 
	time_t now = time(NULL);
	localtime_r(&now, &time_dep);
	time_dep.tm_hour++;
	res = sncf_post_form(curl_hdl, &tdoc, 
		&link, &time_dep, 
		stn_departure, stn_arrival);
	check(res==0, "Failed to perform query");
	debug("Initialized (%d) - link = %s", res, link);

	//Fetch, parse, print
	while(1) {	
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
			log_info("Next results page is the same as the current one (%d successes)", consecutive_success);
			if(consecutive_success <= 2) {
				log_info("less than 3 success before loop, this is the end");
				break;
			}
			localtime_r(&get_last_train(trains)->train.time_departure, &time_dep);

			consecutive_success = 0;
			tidyRelease(tdoc);

			free(link);
			free(new_link);
			res = sncf_post_form(curl_hdl, &tdoc, 
				&link, &time_dep, 
				stn_departure, stn_arrival);
			check(res==0, "Failed to perform query");
			continue;
		}

		free_trains(trains);
		trains = NULL;
		ntrains = sncf_parse_results(db_hdl, tdoc, &trains);
		debug("found %lu trains", ntrains);	
		n = train_store(db_hdl, trains);	
		if(n!=ntrains) {
			log_info("only stored %lu out of %lu trains, aborting", n, ntrains);
			goto error;
		}
		debug("Stored all %lu trains", n);
		total+=n;

#ifdef NDEBUG
		localtime_r(&get_last_train(trains)->train.time_departure, &time_dep);
		strftime(str_time_dep, 20, "%e-%b-%Y %R", &time_dep);
		printf("Processed %6lu trains - Last one departed at %s\r",
			total, str_time_dep); 		
		fflush(stdout);
#else
		print_trains(db_hdl, trains, 0);
#endif
		consecutive_success++; 
		free(link);
		link = new_link;
	}
	free(link);
	free(new_link);

	if(tdoc) {
		tidySaveFile(tdoc, "dumpfile-exit.html");
		tidyRelease(tdoc);
	}
	curl_tidy_cleanup(curl_hdl);
	database_cleanup(db_hdl);
	log_info("Exiting after storing %lu trains", total);
	return 0;

error:
	if(tdoc) {
		tidySaveFile(tdoc, "dumpfile-exit.html");
		tidyRelease(tdoc);
	}
	curl_tidy_cleanup(curl_hdl);
	database_cleanup(db_hdl);
	return -1;
usage:
	printf(
		"Usage : %s -d <dbfile> -f <stn_dep> -t <stn_arr>\n"
		"\n"
		"\t<dbfile>\tThe sqlite3 database filename\n"
		"\t<stn_dep>\tThe departure station\n"
		"\t<stn_arr>\tThe arrival station\n"
		"\n",
	argv[0]);
	return 0;
}
