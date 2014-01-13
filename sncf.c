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
	TidyNode summary, node;
	struct node_list *nodes, *node_cur;
	struct train_info* trains = NULL;
	char *data;
	char *stn_departure, *stn_arrival;
	unsigned int day, month, year;
	struct tm tm_current_day;
	int res, i = 0, ntrains;
	summary = findNodeById(tidyGetRoot(tdoc), "block-bestpricesummary");
	check(summary, "No price summary node found");

	/*
	 * Get the date of the first column from the first <h2>'s <strong>
	 */
	findNodesByName(&nodes, summary, "h2");
	check(nodes, "No <h2> found");
	node = nodes->node; 
	freeNodeList(nodes); 

	findNodesByName(&nodes, node, "strong");
	check(nodes, "No <strong> found");
	getNodeText(tdoc, nodes->node, &data);

	check_mem(data);
	
	res = sscanf(data, "Outward journey between %02u/%02u/%4u",
				&day, &month, &year);

	tm_current_day.tm_hour = tm_current_day.tm_min = tm_current_day.tm_sec = 0;
	tm_current_day.tm_mday = day;
	tm_current_day.tm_mon = month - 1;
	tm_current_day.tm_year = year - 1900;	
	tm_current_day.tm_isdst = -1; //Have mktime figure DST out

	freeNodeList(nodes);	
	free(data);
#if 0
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
#endif
	/*
	 * Get departure and destination from breadcrumb
	 */
	node = findNodeById(tidyGetRoot(tdoc), "breadcrumb");
	check(node, "Breadcrumb node not found");
	res = findNodesByClass(&nodes, node, "actual");
	if(res > 1) log_warn("More than 1 nodes found, Memory leak probably");

	getNodeText(tdoc, nodes->node, &data);
	stn_departure = data + strlen("Your reservation for ");
	stn_departure = strndup(stn_departure, strstr(stn_departure," - ") - stn_departure);
	stn_arrival = strstr(data, " - ") + strlen(" - ");
	stn_arrival = strndup(stn_arrival, strstr(stn_arrival,"\n") - stn_arrival);
	debug("DEP = %s - ARR = %s", stn_departure, stn_arrival);

	freeNodeList(nodes);
	free(data);

	/*
	 * Get all rows in the table
	 */
	findNodesByName(&nodes, summary, "tbody");
	check(nodes, "No <tbody> found");
	node = nodes->node; //We only care about the first find
	freeNodeList(nodes);

	res = findNodesByName(&nodes, node, "tr");
	log_info("found %d table rows", res);
	check(nodes, "No <tr> found");
	 	
	//Go through all rows and parse their <td> into train_info
	for(node_cur = nodes; node_cur; node_cur = node_cur->next) {
		struct node_list *cells, *cell_cur;
 		ntrains = findNodesByName(&cells, node_cur->node, "td");
		debug("Found %d cells in row classed %s", 
			ntrains, getAttributeValue(node_cur->node, "class"));
		if(!trains) {
			debug("allocating trains");
			trains = calloc(ntrains, sizeof(struct train_info));	
			//initialise dept & arr stn as well
			for(int i = 0; i < ntrains; i++) {
				trains[i].stn_departure = stn_departure;
				trains[i].stn_arrival = stn_arrival;
			}
		}

		i = 0;
		for(cell_cur = cells; cell_cur; cell_cur = cell_cur->next) {
			//Test for current day (cd) or day after (da)
			char *cell_text = NULL;
			const char *header_attr = NULL;
			int day_offset = 0;
			header_attr = getAttributeValue(cell_cur->node, "headers");
			if(strstr(header_attr, "da")) day_offset = 1;

			//Get <td> (and children) contents
			cell_text = NULL;
			getNodeText(tdoc, cell_cur->node, &cell_text);
			check_mem(cell_text);

			//add the cell to the appropriate field
			if(testNodeClass(node_cur->node, "departureTime")) {
				struct tm tm_departure = tm_current_day;
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
				//departureTime should be set already
				struct tm tm_arrival;
				int dur_hour, dur_min;
				gmtime_r(&trains[i].time_departure, &tm_arrival);
				res = sscanf(cell_text, "%02dh%02d", 
					&dur_hour,
					&dur_min);	
				check(res == 2, "Failed to parse duration");
				tm_arrival.tm_hour += dur_hour;
				tm_arrival.tm_min += dur_min;
				trains[i].time_arrival = timegm(&tm_arrival);
			} else if(testNodeClass(node_cur->node, "transporteur")) {
				trains[i].operator = strndup(cell_text, 
					strstr(cell_text, "\n") - cell_text);
			} else {
				log_warn("Unhandled cell data");
			}
			free(cell_text);
			i++;
		}

		freeNodeList(cells);
	}
	freeNodeList(nodes);

	/*
	 * Let's print what we found
	 */
	char format[255]= "%15s | %15s | %10s | %10s | %8s | %20s\n";
	printf(format, "Departure", "Arrival", "TOD", "arr", "Price", "Operator");
	printf("-------------------------\n");
	for(i = 0; i < ntrains; i++)
	{
		char time_arr[10], time_dep[10], price[10];
		struct tm tm;	
		localtime_r(&trains[i].time_departure, &tm);
		strftime(time_dep, 10, "%R", &tm);
		localtime_r(&trains[i].time_arrival, &tm);
		strftime(time_arr, 10, "%R", &tm);
		sprintf(price, "%f", trains[i].price);	

		printf(format,
			trains[i].stn_departure,
			trains[i].stn_arrival,
			time_dep,
			time_arr,
			price,
			trains[i].operator
			);

	}	

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


