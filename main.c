#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "include/dbg.h"
#include "postfield.h"

#define _CURLOPT(setting, value) \
		curl_easy_setopt(curl_hdl, CURLOPT_##setting, value);
#define TMP_FN_TEMPLATE "schedrip-XXXXXX"

const char* cookiefile = "cookies.txt";

int main(int argc, char *argv[])
{
	CURL *curl_hdl = NULL;
 	CURLcode curl_res = 0;
 
	char url[255] = "undefined";
	char filename_temp[16];
	FILE *fid_html_index, *fid_html_redirect, *fid_html_results;

	//Create file to dump html to
	//FIXME: drop this file bullshit and use libtidy
	strcpy(filename_temp, TMP_FN_TEMPLATE);
	check(mkstemp(filename_temp), "Failed to get a tmpfile");
	fid_html_index= fopen(filename_temp, "w");
	log_info( " INDEX filename = %s", filename_temp);
	strcpy(filename_temp, TMP_FN_TEMPLATE);
	check(mkstemp(filename_temp), "Failed to get a tmpfile");
	fid_html_redirect = fopen(filename_temp, "w");
	log_info( " REDIR filename = %s", filename_temp);
	strcpy(filename_temp, TMP_FN_TEMPLATE);
	check(mkstemp(filename_temp), "Failed to get a tmpfile");
	fid_html_results = fopen(filename_temp, "w");
	log_info( " RESUL filename = %s", filename_temp);

	//Set up CURL
	log_info("started SNCF ripper");
	curl_hdl = curl_easy_init();
	check(curl_hdl, "failed to initialise curl");
	_CURLOPT(VERBOSE, 0); 
	_CURLOPT(HEADER, 0);
	_CURLOPT(NOPROGRESS, 1);
//	_CURLOPT(USERAGENT, "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_9_1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/31.0.1650.63 Safari/537.36");
//	_CURLOPT(AUTOREFERER, 1);
	_CURLOPT(FOLLOWLOCATION, 1);
//	_CURLOPT(COOKIEFILE, cookiefile); //Read known cookies
//	_CURLOPT(COOKIEFILE, ""); 	  //enable cookies
	_CURLOPT(COOKIEJAR, cookiefile); //Makes curl write cookies back at cleanup
	//All set, let's do this!

#if 0
//-------------- INDEX -------------
	_CURLOPT(HTTPGET, 1);
	_CURLOPT(COOKIESESSION, 1);
	_CURLOPT(WRITEDATA, fid_html_index);	
	strcpy(url,"http://be.voyages-sncf.com/en/");
	_CURLOPT(URL, url);
	curl_res = curl_easy_perform(curl_hdl);
	fclose(fid_html_index);
	check(curl_res == CURLE_OK, "Curl error: %s", curl_easy_strerror(curl_res));
#endif
//--------------- SEND request & get results URI
	_CURLOPT(WRITEDATA, fid_html_redirect);	
	_CURLOPT(HTTPPOST, 1);
//	_CURLOPT(REFERER, "http://be.voyages-sncf.com/en/");
	_CURLOPT(POSTFIELDS, postfields);
//	_CURLOPT(COOKIESESSION, 1);
	strcpy(url,"http://be.voyages-sncf.com/vsc/train-ticket/?_LANG=en");
	_CURLOPT(URL, url);
	curl_res = curl_easy_perform(curl_hdl);
	fclose(fid_html_redirect);
	check(curl_res == CURLE_OK, "Curl error: %s", curl_easy_strerror(curl_res));

//--------------- RESULT ------------------
#if 0
	//TODO: parse url from post page using libtidy & libxml
	//FIXME: use libtidy for the storing and parsing the pages
	_CURLOPT(WRITEDATA, fid_html_results);	
	_CURLOPT(HTTPGET,1);
	strcpy(url, "http://be.voyages-sncf.com:80/vsc/proposals/findProposals?hid=9SFi");
	_CURLOPT(URL, url);	
	curl_res = curl_easy_perform(curl_hdl);
	fclose(fid_html_results);
	check(curl_res == CURLE_OK, "Curl error: %s", curl_easy_strerror(curl_res));
#endif
error: 
	log_info("cleaning up");
	curl_easy_cleanup(curl_hdl);
	return 0; 
}
