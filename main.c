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
	int res, i;
	char *postfields = NULL, *link = NULL;
	CURL *curl_hdl = NULL;
 	TidyDoc tdoc = NULL;
	TidyNode summary;
	//Set up CURL
	log_info("started SNCF ripper");

	//Set up some stuff
	check(curl_http_init(&curl_hdl)==0,"Failed to initialise curl");
	debug("curl_hdl %p", curl_hdl);

#if 1
/*
 * Request first results page
 */
	construct_postfields(curl_hdl, &postfields);
	res = fetch_html_post(curl_hdl, 
		"http://be.voyages-sncf.com/vsc/train-ticket/?_LANG=en",
		postfields, &tdoc);
	if(postfields) free(postfields);
	check(res == 0, "Something went wrong fetching (%d)", res);

	//Find my special one
	link = strdup(
		getAttributeValue(
		findNodeById(tidyGetRoot(tdoc),"url_redirect_proposals"), 
		"href"));
	check(link, "Failed to find a link");

/*
 * Iterate to get a couple of results
 */
	for(i=0; i<10; i++) {
		char * dumpfile_name = NULL;
		//Fetch results page
		res = fetch_html_get(curl_hdl, link, &tdoc);

		//Save page to file
		asprintf(&dumpfile_name, "%s-%d.html", "dumpfile", i);
		check_mem(dumpfile_name);
		res = tidySaveFile(tdoc, dumpfile_name); 
		check(res >= 0, "Failed to save to file");	
		free(dumpfile_name);

		//Find next page URL
		summary = findNodeById(tidyGetRoot(tdoc), "block-bestpricesummary");
		free(link);
		sncf_find_next_results(tdoc, summary, &link);
	}
#else
//Read file
	res = read_html("dumpfile-1.html", &tdoc);
	check(res==0, "Failed to read file");
	sncf_parse_pricesummary(tdoc);
	summary = findNodeById(tidyGetRoot(tdoc), "block-bestpricesummary");
	check(summary, "No price summary found");
//	dumpNode(tdoc, tidyGetRoot(tdoc), 0);	
#endif
error:
	log_info("cleaning up");
	tidyRelease(tdoc);
	return 0;
}
