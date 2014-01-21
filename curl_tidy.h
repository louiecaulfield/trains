#ifndef CURL_HTTP_H
#define CURL_HTTP_H

#include <tidy/tidy.h>
#include <tidy/buffio.h>

//! Initialise curl
/*! 
	\param curl_hdl The created curl handle
	\return 0 for success, -1 upon failure
*/
int curl_tidy_init(CURL **curl_hdl);
//! Destroys the curl handle
/*!
	\param curl_hdl The curl handle that needs to be cleaned up
*/
void curl_tidy_cleanup(CURL *curl_hdl);

//! Read an html file into a tidyHtml document, parse and clean the html
/*!
	\param filename A string containing with the html file name, used by fopen
	\param tdoc The Tidy document where the cleaned up document will be stored 
	\return 0 for successs, -1 upon failure
*/
int curl_tidy_read(char *filename, TidyDoc *tdoc);

//! Sends a HTTP POST request with postfields and returns the html
/*!
	\param curl_hdl The curl handle to use
	\param url A string containing the URL
	\param postfields Url encoded post fields
 	\param tdoc The Tidy document where the cleaned up response will be stored
	\return 0 for success, -1 upon failure
*/
int curl_tidy_post(CURL *curl_hdl, const char * url, char* postfields, TidyDoc *tdoc);

//! Sends a HTTP GET request and returns the html
/*!
	\param curl_hdl The curl handle to use
	\param url A string containing the URL
 	\param tdoc The Tidy document where the cleaned up response will be stored
	\return 0 for success, -1 upon failure
*/
int curl_tidy_get(CURL *curl_hdl, const char * url, TidyDoc *tdoc);

#endif
