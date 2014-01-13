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
	size_t ntrains;

	//Set up some stuff
	check(curl_http_init(&curl_hdl)==0,"Failed to initialise curl");
	debug("curl_hdl %p", curl_hdl);

	//Send search query 
	res = sncf_post_form(curl_hdl, &tdoc, &link);
	log_info("Initialized (%d) - link = %s", res, link);
	tidyRelease(tdoc);

	//Fetch, parse, print
	for(int i = 0; i < 10; i++) {
		res = fetch_html_get(curl_hdl, link, &tdoc);	
		check(res ==  0, "failed to fetch results page");

		ntrains = sncf_parse_train_info(tdoc, &trains);
		check(ntrains, "No trains found");
		sncf_print_train_info(trains, ntrains, 0);
		sncf_free_train_info(&trains, &ntrains);

		free(link);
		sncf_find_next_results(tdoc, &link);
		tidyRelease(tdoc);
	}
	free(link);
error:
	//Dump last download (in case of error)
	tidySaveFile(tdoc, "dumpfile-exit.html");
	return 0;
}
