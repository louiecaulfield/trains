#include <stdlib.h>
#include <curl/curl.h>
#include <tidy/tidy.h>
#include "include/dbg.h"
#include "sncf.h"
#include "sncf_cities.h"
#include "html.h"

static char * postfields_default = 
	"site_country=BE&"
	"site_language=en&"
	"form_build_id=form-6d982756d10276d8c72448fdccdc3592&"
	"form_id=_vsct_feature_booking_train_form&"
	"INWARD_DATE=&"
	"INWARD_TIME=7&"
	"COMFORT_CLASS=2&"
	"DIRECT_TRAVEL_CHECK=1&"
	"NB_TYPO_ADULT=1&"
	"PASSENGER_1=ADULT&"
	"PASSENGER_1_CARD=default&"
	"PASSENGER_1_FID_PROG=default&"
	"PASSENGER_1FID_NUM_BEGIN=&"
	"PASSENGER_2=ADULT&"
	"PASSENGER_2_CARD=default&"
	"PASSENGER_2_FID_PROG=default&"
	"PASSENGER_2FID_NUM_BEGIN=&"
	"PASSENGER_3=ADULT&"
	"PASSENGER_3_CARD=default&"
	"PASSENGER_3_FID_PROG=default&"
	"PASSENGER_3FID_NUM_BEGIN=&"
	"PASSENGER_4=ADULT&"
	"PASSENGER_4_CARD=default&"
	"PASSENGER_4_FID_PROG=default&"
	"PASSENGER_4FID_NUM_BEGIN=&"
	"PASSENGER_5=ADULT&"
	"PASSENGER_5_CARD=default&"
	"PASSENGER_5_FID_PROG=default&"
	"PASSENGER_5FID_NUM_BEGIN=&"
	"PASSENGER_6=ADULT&"
	"PASSENGER_6_CARD=default&"
	"PASSENGER_6_FID_PROG=default&"
	"PASSENGER_6FID_NUM_BEGIN=&"
	"pacReducCard1=default&"
	"pacReducCard2=default&"
	"pacReducCard3=default&"
	"pacReducCard4=default&"
	"pacReducCard5=default&"
	"pacReducCard6=default&"
	"bookingChoice=train&"
	"DISTRIBUTED_COUNTRY=BE&"
	"action%3AsearchTravel=SEARCH";

int construct_postfields(CURL *curl_hdl, char ** postfields)
{
	int res;
	char *city_origin, *city_dest;//, *out_date, *out_time;
	city_origin = curl_easy_escape(curl_hdl, sncf_cities[19 -5], 0);
	city_dest = curl_easy_escape(curl_hdl, sncf_cities[30-5], 0);

	res = asprintf(postfields,
		"%s%s%s%s%s%s%s%s%s%s",
		"ORIGIN_CITY=", city_origin,
		"&DESTINATION_CITY=", city_dest,
		"&OUTWARD_DATE=", "04%2F02%2F2014",
		"&OUTWARD_TIME=", "15",
		"&", postfields_default);
	curl_free(city_origin);
	curl_free(city_dest);
	check(res>=0,"Failed to construct postfields");
	return 0;
error:
	free(postfields);
	return -1;
}	
int sncf_parse_pricesummary(TidyDoc tdoc)
{
	TidyNode summary, row_days = NULL;
	struct node_list *nodes, *node_cur;
	int i, days;
	summary = findNodeById(tidyGetRoot(tdoc), "block-bestpricesummary");
	check(summary, "No price summary found");

	//Find out how many colums we have
	nodes = findNodesByClass(summary, "dayInfo");

	if(nodes) row_days = nodes->node;
	nodes = findNodesByName(row_days, "th");

	for(node_cur = nodes; node_cur; node_cur = node_cur->next) {
		TidyAttr colspan = findAttribute(node_cur->node, "colspan");
		check(colspan, "OMG no there's no colspan!");
		const char * days = tidyAttrValue(colspan);
		log_info("got colspan %d", atoi(days));
	}	
	freeNodeList(nodes);

//	th_days = findNodeByNameAndClass(row_days, "th", "k	
//	dumpNode(tdoc, tidyGetRoot(tdoc), 0);	
	return 0;
error:
	return -1;
}
