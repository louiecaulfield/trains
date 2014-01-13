#ifndef SNCF_H
#define SNCF_H

int construct_postfields(CURL *curl_hdl, char ** postfields);

int sncf_parse_pricesummary(TidyDoc tdoc);
#endif
