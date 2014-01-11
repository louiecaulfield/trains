#include <curl/curl.h>
#include "include/dbg.h"
#include "curl_http.h"
#include "html.h"

#define _CURLOPT(setting, value) \
		curl_easy_setopt(curl_hdl, CURLOPT_##setting, value);

//FIXME: this should be removed evnetually
const char* cookiefile = "cookies.txt";

//char curl_errbuf[CURL_ERROR_SIZE];

size_t write_cb(char *in, size_t size, size_t nmemb, TidyBuffer *out)
{
	size_t r;
	r = size * nmemb;
	tidyBufAppend(out, in, r);
	return(r);
}

int curl_http_init(CURL **curl_hdl_ref)
{
	CURL *curl_hdl;
	*curl_hdl_ref = curl_easy_init();
	curl_hdl = *curl_hdl_ref;
	check(curl_hdl, "failed to initialise curl");

	_CURLOPT(VERBOSE, 0);
	_CURLOPT(HEADER, 0);
	_CURLOPT(NOPROGRESS, 1);
	_CURLOPT(FOLLOWLOCATION, 1);
//	FIXME: Stop using cookiejar when ready, or at least make it configurable
//	_CURLOPT(COOKIEFILE, ""); 	  //enable cookies
	_CURLOPT(COOKIEJAR, cookiefile); //Makes curl write cookies back at cleanup
	_CURLOPT(WRITEFUNCTION, write_cb);

	return 0;

error:
	return -1;
}

void curl_http_cleanup(CURL *curl_hdl)
{
	curl_easy_cleanup(curl_hdl);
}

int fetch_html(CURL *curl_hdl, const char * url, TidyDoc *tdoc)
{
	TidyBuffer docbuf = {0};
	TidyBuffer tidy_errbuf = {0};
	_CURLOPT(URL, url);

	*tdoc = tidyCreate();
	tidyOptSetBool(*tdoc, TidyForceOutput, yes);
	tidyOptSetInt(*tdoc, TidyWrapLen, 4096);
	tidySetErrorBuffer(*tdoc, &tidy_errbuf);
	tidyBufInit(&docbuf);

	_CURLOPT(WRITEDATA, &docbuf);

	check(curl_easy_perform(curl_hdl)==0, "Curl request failed");
	log_info("Fetched the page");
	check(tidyParseBuffer(*tdoc, &docbuf)>=0, "Failed to parse buffer");
	log_info("Parse complete");
	check(tidyCleanAndRepair(*tdoc)>=0, "Failed to clean/repair html");
	log_info("Cleanup done");
	tidyBufFree(&docbuf);
	tidyBufFree(&tidy_errbuf);

	return 0;
error:
	tidyBufFree(&docbuf);
	tidyBufFree(&tidy_errbuf);
	return -1;
}

int fetch_html_get(CURL *curl_hdl, const char * url, TidyDoc *tdoc)
{
	check(curl_hdl, "Curl not initialised!");
	_CURLOPT(HTTPGET,1);
	fetch_html(curl_hdl, url, tdoc);
error:
	return -1;
}

int fetch_html_post(CURL *curl_hdl, const char * url, char* postfields, TidyDoc *tdoc)
{ 
	int res;
	check(curl_hdl, "Curl not initialised!");
	_CURLOPT(HTTPPOST, 1);
	_CURLOPT(POSTFIELDS, postfields);
	res = fetch_html(curl_hdl, url, tdoc);
	return res; 
error:
	return -1;

}

