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
	char * postfields = NULL;
	CURL *curl_hdl = NULL;
 	TidyDoc tdoc = NULL;
	TidyNode link, summary;
	//Set up CURL
	log_info("started SNCF ripper");

//Set up some stuff
	check(curl_http_init(&curl_hdl)==0,"Failed to initialise curl");
	debug("curl_hdl %p", curl_hdl);
#if 1
//--------------- SEND request & get results URI
	construct_postfields(curl_hdl, &postfields);
	res = fetch_html_post(curl_hdl, 
		"http://be.voyages-sncf.com/vsc/train-ticket/?_LANG=en",
		postfields, &tdoc);
	if(postfields) free(postfields);
	check(res == 0, "Something went wrong fetching (%d)", res);

	//Find my special one
	link = findNodeById(tidyGetRoot(tdoc),"url_redirect_proposals");
	check(link, "Failed to find a link");
	log_info("Results page = %s", tidyAttrValue(findAttribute(link, "href")));
//--------------- RESULT ------------------
	//TODO: parse url from post page using libtidy & libxml
	res = fetch_html_get(curl_hdl, tidyAttrValue(findAttribute(link, "href")), &tdoc);
	log_info("Got the results page");
	res = tidySaveFile(tdoc, "dumpfile.html");
	check(res >= 0, "Failed to save to file");	
#endif
//Read file
	res = read_html("dumpfile.html", &tdoc);
	check(res==0, "Failed to read file");
	summary = findNodeById(tidyGetRoot(tdoc), "block-bestpricesummary");
	check(summary, "No price summary found");
	dumpNode(tdoc, tidyGetRoot(tdoc), 0);	
error:
	log_info("cleaning up");
	tidyRelease(tdoc);
	return 0;
}
