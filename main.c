#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <tidy/tidy.h>
#include <tidy/buffio.h>
#include "include/dbg.h"
#include "postfield.h"

#define _CURLOPT(setting, value) \
		curl_easy_setopt(curl_hdl, CURLOPT_##setting, value);
#define TMP_FN_TEMPLATE "schedrip-XXXXXX"

const char* cookiefile = "cookies.txt";

size_t write_cb(char *in, size_t size, size_t nmemb, TidyBuffer *out)
{
	size_t r;
	r = size * nmemb;
	tidyBufAppend(out, in, r);
	return(r);
}

void dumpNode(TidyNode tnod, int indent)
{
	TidyNode child;
	for(child = tidyGetChild(tnod); child; child = tidyGetNext(child))
	{
		ctmbstr name = tidyNodeGetName(child);
		if(name)
		{
			//if it has a name, it's a tag
			TidyAttr attr;
			printf("%*.*s%s ", indent, indent, "<", name);
			// walk attr list
			for(attr = tidyAttrFirst(child); attr; attr=tidyAttrNext(attr)) {
				printf("%s", tidyAttrName(attr));
				tidyAttrValue(attr)?printf("=\"%s\" ",
					tidyAttrValue(attr)):printf(" ");
			}
			printf(">\n");
		}
		dumpNode(child, indent + 4);
	}
}

TidyAttr findAttribute(TidyNode node, char * attrname)
{
	TidyAttr attr;
	for(attr = tidyAttrFirst(node); attr; attr=tidyAttrNext(attr)) {
		//FIXME: use strncmp instead?
		if(!strcmp(attrname, tidyAttrName(attr))) {
			return attr;
		}
	}
	return NULL;
}

TidyNode findNodeById(TidyNode node, char * id)
{
	TidyNode child, match = NULL;
	TidyAttr attr;
	for(child = tidyGetChild(node); child; child = tidyGetNext(child)) {
		if( (attr = findAttribute(child, "id") ) ) {
			if(!strcmp(id, tidyAttrValue(attr))) {
				//Match!
				debug("found a match!");
				return child;
			}
		}
		if ( (match = findNodeById(child, id)) ) {
			return match;
		}
	}
	return NULL;
}

int main(int argc, char *argv[])
{
	CURL *curl_hdl = NULL;
 	CURLcode curl_res = 0;
	int res = 0;
//	char url[255] = "undefined";
	char curl_errbuf[CURL_ERROR_SIZE];

	TidyDoc tdoc = NULL;
	TidyBuffer docbuf = {0};
	TidyBuffer tidy_errbuf = {0};
	TidyNode link; //matching link to redirect!

#if 0
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
#endif


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
	//FIXME: Stop using cookiejar when ready, or at least make it configurable
//	_CURLOPT(COOKIEFILE, ""); 	  //enable cookies
	_CURLOPT(COOKIEJAR, cookiefile); //Makes curl write cookies back at cleanup
	_CURLOPT(WRITEFUNCTION, write_cb);
	//All set, let's do this!

//--------------- SEND request & get results URI
//	_CURLOPT(WRITEDATA, fid_html_redirect);
	_CURLOPT(HTTPPOST, 1);
	_CURLOPT(POSTFIELDS, postfields);
//	strcpy(url,"http://be.voyages-sncf.com/vsc/train-ticket/?_LANG=en");
	_CURLOPT(URL, "http://be.voyages-sncf.com/vsc/train-ticket/?_LANG=en");

	tdoc = tidyCreate();
	tidyOptSetBool(tdoc, TidyForceOutput, yes);
	tidyOptSetInt(tdoc, TidyWrapLen, 4096);
	tidySetErrorBuffer(tdoc, &tidy_errbuf);
	tidyBufInit(&docbuf);

	_CURLOPT(WRITEDATA, &docbuf);
	curl_res = curl_easy_perform(curl_hdl);
//	fclose(fid_html_redirect);
//	check(curl_res == CURLE_OK, "Curl error: %s", curl_easy_strerror(curl_res));
	if(!curl_res) {
		log_info("Fetched the page");
		res = tidyParseBuffer(tdoc, &docbuf);
		if( res >= 0) {
			log_info("Parse complet");
			res = tidyCleanAndRepair(tdoc);
			if( res >= 0 ) {
				log_info("Cleanup done");
				res = tidyRunDiagnostics(tdoc);
				if ( res >= 0 )	{
					log_info("Dumping from the root on");
					dumpNode(tidyGetRoot(tdoc), 0);
					log_info("Errors found:");
					fprintf(stderr, "%s\n", tidy_errbuf.bp);
				}
				//Find my special one
				link = findNodeById(tidyGetRoot(tdoc),"url_redirect_proposals");
				check(link, "Failed to find a link");
				log_info("Results page = %s", tidyAttrValue(findAttribute(link, "href")));
				dumpNode(link, 0);
			}
		}
	} else {
		fprintf(stderr, "%s\n", curl_errbuf);
	}

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
	tidyBufFree(&docbuf);
	tidyBufFree(&tidy_errbuf);
	tidyRelease(tdoc);
	return 0;
}
