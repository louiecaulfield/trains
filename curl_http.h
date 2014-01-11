#ifndef CURL_HTTP_H
#define CURL_HTTP_H

#include <tidy/tidy.h>
#include <tidy/buffio.h>

size_t write_cb(char *in, size_t size, size_t nmemb, TidyBuffer *out);

int curl_http_init();
void curl_http_cleanup();

int fetch_html_post(const char * url, const char* postfields, TidyDoc *tdoc);

int fetch_html_get(const char * url, TidyDoc *tdoc);

#endif
