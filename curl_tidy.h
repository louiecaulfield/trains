#ifndef CURL_HTTP_H
#define CURL_HTTP_H

#include <tidy/tidy.h>
#include <tidy/buffio.h>

size_t write_cb(char *in, size_t size, size_t nmemb, TidyBuffer *out);

int curl_tidy_init(CURL **curl_hdl);
void curl_tidy_cleanup(CURL *curl_hdl);

int curl_tidy_read(char *filename, TidyDoc *tdoc);

int curl_tidy_post(CURL *curl_hdl, const char * url, char* postfields, TidyDoc *tdoc);

int curl_tidy_get(CURL *curl_hdl, const char * url, TidyDoc *tdoc);

#endif
