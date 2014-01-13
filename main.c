#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <tidy/tidy.h>
#include <tidy/buffio.h>
#include "include/dbg.h"
#include "html.h"
#include "curl_http.h"
#include "sncf.h"

#define TMP_FN_TEMPLATE "schedrip-XXXXXX"

int main(int argc, char *argv[])
{
	int res;
	CURL *curl_hdl = NULL;
 	TidyDoc tdoc = NULL;
	char * link;
	struct train_info *trains = NULL;
	int ntrains;

	//Set up some stuff
	check(curl_http_init(&curl_hdl)==0,"Failed to initialise curl");
	debug("curl_hdl %p", curl_hdl);

	//Send search query 
	res = sncf_post_form(curl_hdl, &tdoc, &link);
	log_info("Initialized (%d) - link = %s", res, link);

	//Fetch, parse, print
	for(int i = 0; i < 3; i++) {
		res = fetch_html_get(curl_hdl, link, &tdoc);	
		check(res ==  0, "failed to fetch results page");

		ntrains = sncf_parse_train_info(tdoc, &trains);
		sncf_print_train_info(trains, ntrains);
		free(trains); trains = NULL;
		sncf_find_next_results(tdoc, &link);
	}
error:
	log_info("cleaning up");
	tidyRelease(tdoc);
	return 0;
}
