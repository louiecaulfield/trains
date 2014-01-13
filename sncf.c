#include <stdlib.h>
#include <curl/curl.h>
#include <tidy/tidy.h>
#include <time.h>
#include "include/dbg.h"
#include "sncf.h"
#include "sncf_cities.h"
#include "html.h"

static char * postfields_default;

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

int sncf_find_next_results(TidyDoc tdoc, TidyNode summary, char ** link)
{
	struct node_list *nodes;
	int res;

	res = findNodesByClass(&nodes, summary, "trainsNext");
	if(res > 1) log_warn("Found more than 1 elements, memory leaking!");
	check(nodes, "Couldn't find trainsNext");

	res = findNodesByName(&nodes, nodes->node, "a");
	if(res > 1) log_warn("Found more than 1 elements, memory leaking!");
	check(nodes, "Couldn't find link element");
	*link = strdup(getAttributeValue(nodes->node, "href"));

	freeNodeList(nodes);
	return 0;
error:
	if(nodes) freeNodeList(nodes);
	return -1;
}

int sncf_parse_pricesummary(TidyDoc tdoc)
{
	TidyNode summary, row_days = NULL;
	struct node_list *nodes, *node_cur;
	struct train_info* trains = NULL;
	char *data = NULL;
	unsigned int day, month, year;
	struct tm tm_parse;
	time_t time_first_date;
	int res, i, prices = 0;
	summary = findNodeById(tidyGetRoot(tdoc), "block-bestpricesummary");
	check(summary, "No price summary node found");

	/*
	 * Get the date of the first column from the first <h2>'s <strong>
	 */

	findNodesByName(&nodes, summary, "h2");
	check(nodes, "No <h2> found");
	node_cur = nodes; 
	freeNodeList(nodes); 
	findNodesByName(&nodes, node_cur->node, "strong");
	check(nodes, "No <strong> found");
	getNodeText(tdoc, nodes->node, &data);
	log_info("DATA: %s", data);
	
	res = sscanf(data, "Outward journey between %02u/%02u/%4u",
				&day, &month, &year);
	log_info("Got %d asses (%u/%u/%u)", res, day, month, year);
	tm_parse.tm_hour = tm_parse.tm_min = tm_parse.tm_sec = 1;
	tm_parse.tm_mday = day;
	tm_parse.tm_mon = month - 1;
	tm_parse.tm_year = year - 1900;	
	tm_parse.tm_isdst = -1; //Have mktime figure DST out
	time_first_date = mktime(&tm_parse);

	free(data);
	freeNodeList(nodes);	

	/*
	 * Find out how many colums we have by adding the
	 * colspan values of the <th> in the table header
	 */
	
	res = findNodesByClass(&nodes, summary, "dayInfo");
	check(nodes, "No dayInfo node found");
	if(res > 1) log_warn("Found more than 1 (%d) dayInfo element!", res);
	row_days = nodes->node;
	freeNodeList(nodes);

	res = findNodesByName(&nodes, row_days, "th");
	for(node_cur = nodes; node_cur; node_cur = node_cur->next) {
		const char *colspan;
		colspan = getAttributeValue(node_cur->node, "colspan");
		check(colspan, "Missing colspan on dayInfo <th>");
		log_info("got colspan %d", atoi(colspan));
		prices += atoi(colspan);
	}	
	log_info("Got %d prices to parse", prices);
	freeNodeList(nodes);

	/*
	 * Get all rows in the table
	 */
	findNodesByName(&nodes, summary, "tbody");
	check(nodes, "No <tbody> found");
	node_cur = nodes; //We only care about the first find
	freeNodeList(nodes);
	res = findNodesByName(&nodes, node_cur->node, "tr");
	log_info("found %d table rows", res);
	check(nodes, "No <tr> found");
	 	
	//Go through all rows and parse their <td> into train_info
	trains = calloc(prices, sizeof(struct train_info));	
	i = 0;
	for(node_cur = nodes; node_cur; node_cur = node_cur->next) {
		struct node_list *cells, *cell_cur;
		char *cell_text = NULL;
 		res = findNodesByName(&cells, node_cur->node, "td");
		log_info("Found %d cells in row classed %s", 
			res, getAttributeValue(node_cur->node, "class"));
		for(cell_cur = cells; cell_cur; cell_cur = cell_cur->next) {
			cell_text = NULL;
			getNodeText(tdoc, cell_cur->node, &cell_text);

			//add the cell to the appropriate field
			if(testNodeClass(node_cur->node, "departureTime")) {
				log_info("adding deptime %s", cell_text); 
			} else if(testNodeClass(node_cur->node, "price")) {
				log_info("adding price %s", cell_text); 
			} else if(testNodeClass(node_cur->node, "duration")) {
				log_info("adding duration %s", cell_text); 
			} else if(testNodeClass(node_cur->node, "transporteur")) {
				log_info("adding transporter %s", cell_text); 
			} else {
				log_warn("Unhandled cell data");
			}
			free(cell_text);
		}

		freeNodeList(cells);
	}
	freeNodeList(nodes);
//	th_days = findNodeByNameAndClass(row_days, "th", "k	
//	dumpNode(tdoc, tidyGetRoot(tdoc), 0);	
	return 0;
error:
	return -1;
}

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


