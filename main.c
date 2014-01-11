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
	TidyNode link;
	//Set up CURL
	log_info("started SNCF ripper");

//--------------- SEND request & get results URI
	check(curl_http_init(&curl_hdl)==0,"Failed to initialise curl");
	debug("curl_hdl %p", curl_hdl);
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
error:
	log_info("cleaning up");
	tidyRelease(tdoc);
	return 0;
}
