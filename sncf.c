#include <stdlib.h>
#include <curl/curl.h>
#include <tidy/tidy.h>
#include <time.h>
#include <string.h>
#include <sqlite3.h>
#include "include/dbg.h"
#include "sncf.h"
#include "sncf_stations.h"
#include "html.h"
#include "curl_tidy.h"
#include "database.h"

static char * postfields_default;

int construct_postfields(CURL *curl_hdl, char ** postfields, 
	struct tm *time_departure, 
	const char *stn_departure, const char *stn_arrival)
{
	int res;
	char *stn_departure_esc, *stn_arrival_esc, out_date[16], out_time[4];

	stn_departure_esc = curl_easy_escape(curl_hdl, stn_departure, 0);
	stn_arrival_esc = curl_easy_escape(curl_hdl, stn_arrival, 0);

	strftime(out_date, 16, "%d%%2F%m%%2F%Y", time_departure);	
	strftime(out_time, 4, "%H", time_departure);	
	debug("out_date = [%s] ; out_time = [%s]", out_date, out_time);

	res = asprintf(postfields,
		"%s%s%s%s%s%s%s%s%s%s",
		"ORIGIN_CITY=", stn_departure_esc,
		"&DESTINATION_CITY=", stn_arrival_esc,
		"&OUTWARD_DATE=", out_date, 
		"&OUTWARD_TIME=", out_time, 
		"&", postfields_default);
	curl_free(stn_departure_esc);
	curl_free(stn_arrival_esc);
	check(res>=0,"Failed to construct postfields");
	return 0;
error:
	free(postfields);
	return -1;
}	

/*
 * Request first results page
 */
int sncf_post_form(CURL *curl_hdl, TidyDoc *tdoc, char ** link, 
	struct tm *time_departure, 
	char *stn_departure, char *stn_arrival)
{
	char *postfields = NULL;
	int res;
	construct_postfields(curl_hdl, &postfields, 
		time_departure, stn_departure, stn_arrival);
	res = curl_tidy_post(curl_hdl, 
		"http://be.voyages-sncf.com/vsc/train-ticket/?_LANG=en",
		postfields, tdoc);
	if(postfields) free(postfields);
	check(res == 0, "Something went wrong fetching (%d)", res);

	//Find my special one
	*link = strdup(
		getAttributeValue(
		findNodeById(tidyGetRoot(*tdoc),"url_redirect_proposals"), 
		"href"));
	check(*link, "Failed to find a link");

	return 0;
error:
	return -1;
}

int sncf_find_next_results_link(TidyDoc tdoc, char ** link)
{
	TidyNode summary, node;
	struct node_list *nodes = NULL;
 	int res;
	
	summary = findNodeById(tidyGetRoot(tdoc), "block-bestpricesummary");
	check(summary, "No summary node found");

	res = findNodesByClass(&nodes, summary, "trainsNext");
	if(res > 1) log_warn("Found more than 1 elements");
	check(nodes, "Couldn't find trainsNext");
	node = nodes->node;
	freeNodeList(&nodes);
	
	res = findNodesByName(&nodes, node, "a");
	if(res > 1) log_warn("Found more than 1 elements");
	check(nodes, "Couldn't find link element");
	*link = strdup(getAttributeValue(nodes->node, "href"));

	freeNodeList(&nodes);
	return 0;
error:
	if(nodes) freeNodeList(&nodes);
	return -1;
}

size_t sncf_parse_results(sqlite3 *db_hdl, TidyDoc tdoc, struct train_t **ret)
{
	TidyNode summary, node;
	struct node_list *nodes = NULL, *node_cur = NULL;
	struct node_list *cells = NULL, *cell_cur = NULL;
	struct train_t *trains = NULL;
	char *data = NULL;
	char *stn_departure = NULL, *stn_arrival = NULL;
	int stn_departure_id, stn_arrival_id;
	char *cell_text = NULL;
	const char *header_attr = NULL;
	unsigned int day, month, year;
	struct tm tm_current_day;
	int res, i = 0;
	size_t ntrains = 0;
	int day_offset, day_before_encountered;

	/*
	 * Get departure and arrival from breadcrumb
	 */
	node = findNodeById(tidyGetRoot(tdoc), "breadcrumb");
	check(node, "Breadcrumb node not found");
	res = findNodesByClass(&nodes, node, "actual");
	if(res > 1) log_warn("More than 1 nodes found where only 1 expected");

	getNodeText(tdoc, nodes->node, &data);
	stn_departure = data + strlen("Your reservation for ");
	stn_departure = strndup(stn_departure, strstr(stn_departure," - ") - stn_departure);
	stn_arrival = strstr(data, " - ") + strlen(" - ");
	stn_arrival = strndup(stn_arrival, strstr(stn_arrival,"\n") - stn_arrival);
	debug("DEP = %s - ARR = %s", stn_departure, stn_arrival);

	freeNodeList(&nodes);
	free(data);

	/*
	 * Find the node containing the summary table
	 */
	summary = findNodeById(tidyGetRoot(tdoc), "block-bestpricesummary");
	check(summary, "No price summary node found");

	/*
	 * Get the date of the first column from the first <h2>'s <strong>
	 */
	findNodesByName(&nodes, summary, "h2");
	check(nodes, "No <h2> found");
	node = nodes->node; 
	freeNodeList(&nodes); 

	findNodesByName(&nodes, node, "strong");
	check(nodes, "No <strong> found");
	getNodeText(tdoc, nodes->node, &data);
	freeNodeList(&nodes);	

	check_mem(data);
	res = sscanf(data + (strstr(data,"Outward")?
				strlen("Outward journey between "):
				strlen("Leaving on ")), "%02u/%02u/%4u",
				&day, &month, &year);

	tm_current_day.tm_hour = tm_current_day.tm_min = tm_current_day.tm_sec = 0;
	tm_current_day.tm_mday = day;
	tm_current_day.tm_mon = month - 1;
	tm_current_day.tm_year = year - 1900;	
	tm_current_day.tm_isdst = -1; //Have mktime figure DST out
	free(data);

	/*
	 * Get all rows in the table
	 */
	findNodesByName(&nodes, summary, "tbody");
	check(nodes, "No <tbody> found");
	node = nodes->node; //We only care about the first find
	freeNodeList(&nodes);

	/*
	 * Go through all <tr> and parse their <td> into train_t
	 */
	res = findNodesByName(&nodes, node, "tr");
	check(res == 4, "didn't find required 4 <tr> nodes");
	for(node_cur = nodes; node_cur; node_cur = node_cur->next) {
		ntrains = findNodesByName(&cells, node_cur->node, "td");
		debug("Found %lu cells in row classed %s", 
			ntrains, getAttributeValue(node_cur->node, "class"));
		if(!trains) {
			debug("allocating trains");
			trains = calloc(ntrains, sizeof(struct train_t));	
			check_mem(trains);
			//initialise dept & arr stn as well
			station_find(db_hdl, stn_departure, NULL, &stn_departure_id);
			station_find(db_hdl, stn_arrival, NULL, &stn_arrival_id);
			for(size_t i = 0; i < ntrains; i++) {
				trains[i].stn_departure = stn_departure_id; 
				trains[i].stn_arrival = stn_arrival_id; 
			}
		}

		i = 0;
		day_before_encountered = 0;
		for(cell_cur = cells; cell_cur; cell_cur = cell_cur->next) {
			//Get <td> (and children) contents
			getNodeText(tdoc, cell_cur->node, &cell_text);
			check_mem(cell_text);

			//add the cell to the appropriate field
			if(testNodeClass(node_cur->node, "departureTime")) {
				struct tm tm_departure = tm_current_day;

				//Test for day before (db) current day (cd) or day after (da)
				header_attr = getAttributeValue(cell_cur->node, "headers");
				if(strstr(header_attr, "db")) {
					day_before_encountered = 1;
					day_offset = 0;
				} else if(strstr(header_attr, "cd")) {
					day_offset = day_before_encountered;
				} else if(strstr(header_attr, "da")) {
					day_offset = 1 + day_before_encountered;
				} else {
					log_err("Didn't find day specified in <td header>");
					goto error;
				}

				res = sscanf(cell_text, "%02dh%02d", 
					&tm_departure.tm_hour,
					&tm_departure.tm_min);		
				tm_departure.tm_mday += day_offset;
				trains[i].time_departure = mktime(&tm_departure);
				check(res == 2, "failed to assign time");
			} else if(testNodeClass(node_cur->node, "price")) {
				float price;
				res = sscanf(cell_text, "%f", &price);
				check(res == 1, "too many prices found");	
				trains[i].price = price;
			} else if(testNodeClass(node_cur->node, "duration")) {
				struct tm tm_arrival;
				int dur_hour, dur_min;
				//departureTime should be set already
				gmtime_r(&trains[i].time_departure, &tm_arrival);
				res = sscanf(cell_text, "%02dh%02d", 
					&dur_hour,
					&dur_min);	
				check(res == 2, "Failed to parse duration");
				tm_arrival.tm_hour += dur_hour;
				tm_arrival.tm_min += dur_min;
				trains[i].time_arrival = timegm(&tm_arrival);
			} else if(testNodeClass(node_cur->node, "transporteur")) {
				//Cut off operator at first \n
				trains[i].operator = strndup(cell_text, 
					strstr(cell_text, "\n") - cell_text);
			} else {
				log_warn("Unhandled cell data");
			}
			free(cell_text);
			cell_text = NULL;
			i++;
		}
		freeNodeList(&cells);
	}
	freeNodeList(&nodes);
	goto success;
error:
	free_trains(&trains, &ntrains);	
	free(cell_text);
	freeNodeList(&cells);
	freeNodeList(&nodes);
success:
	free(stn_departure);
	free(stn_arrival);
	*ret = trains;
	debug("returning from parse");
	return ntrains;

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


