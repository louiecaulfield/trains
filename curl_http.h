#ifndef CURL_HTTP_H
#define CURL_HTTP_H

#include <tidy/tidy.h>
#include <tidy/buffio.h>

size_t write_cb(char *in, size_t size, size_t nmemb, TidyBuffer *out);

int curl_http_init(CURL **curl_hdl);
void curl_http_cleanup(CURL *curl_hdl);

int read_html(char *filename, TidyDoc *tdoc);

int fetch_html_post(CURL *curl_hdl, const char * url, char* postfields, TidyDoc *tdoc);

int fetch_html_get(CURL *curl_hdl, const char * url, TidyDoc *tdoc);

#endif
