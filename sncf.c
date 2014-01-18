#include <stdlib.h>
#include <curl/curl.h>
#include <tidy/tidy.h>
#include <time.h>
#include <string.h>
#include <sqlite3.h>
#include "include/dbg.h"
#include "include/util.h"
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
	const char *stn_departure, const char *stn_arrival)
{
	char *postfields = NULL;
	const char *_link;
	int res;
	construct_postfields(curl_hdl, &postfields, 
		time_departure, stn_departure, stn_arrival);
	res = curl_tidy_post(curl_hdl, 
		"http://be.voyages-sncf.com/vsc/train-ticket/?_LANG=en",
		postfields, tdoc);
	if(postfields) free(postfields);
	check(res == 0, "Something went wrong fetching (%d)", res);

	//Find my special one
	_link = getAttributeValue(
			findNodeById(tidyGetRoot(*tdoc),
				"url_redirect_proposals"), 
				"href");
	*link = _link?strdup(_link):NULL;
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

size_t sncf_parse_results(sqlite3 *db_hdl, TidyDoc tdoc, struct train_list_t **ret)
{
	TidyNode node, n_hour, n_duration,
		n_station, n_transporter, n_train_nr;
	struct node_list *n_trains = NULL, *n_train_cur;
	struct node_list *nodes, *node_cur;
	struct train_list_t *train = NULL, *trains_head = NULL, *train_new;
	size_t n = 0;

	const char *data = NULL;
	char str_debug[1024] = "";
	char *node_text = NULL;


	int res, i = 0;
	struct tm tm_outward_date, tm_departure, tm_arrival;
	int dur_h, dur_m;
	int stn_dep_id, stn_arr_id;
	int train_nr;
	float price = 10000;
	char *operator;

	/*
	 * Get the date of the first column from the first <h2>'s <strong>
	 */
	node = findNodeById(tidyGetRoot(tdoc), "OUTWARD_DATE");
	check(node, "No node with the outward date found");
	data = getAttributeValue(node, "value");
	strptime(data, "%d/%m/%Y", &tm_outward_date);
	tm_outward_date.tm_hour = tm_outward_date.tm_min = tm_outward_date.tm_sec = 0;

	strftime(str_debug, 1024, "%D %T", &tm_outward_date);
	debug("Outward date found : %s", str_debug);

	node = findNodeById(tidyGetRoot(tdoc), "proposals");
	check(node, "No train proposals found");
	n = findNodesByClass(&n_trains, node, "train_info");
	debug("found %lu trains to parse", n);		

	//price proposals
	for(n_train_cur = n_trains; n_train_cur; n_train_cur = n_train_cur->next) {
		price = 10000;
		stn_dep_id = stn_arr_id = 0;
		//Check if we're dealing with a direct train
		res = findNodesByClass(&nodes, n_train_cur->node, "travel_resume_detail");
		check(res==1,"found no or more than 1 travel_resume_detail node");
		if(!testNodeClass(nodes->node, "direct")) {
			log_info("Ignoring node, indirect train");	
			continue;
		}
		freeNodeList(&nodes);

		tm_departure = tm_outward_date;
		//Get the day
		node = findNodeNByClass(n_train_cur->node, 1, "day-info");
		check(node, "node day_info not found");
		getNodeText(tdoc, node, &node_text);
		strptime(strstr(node_text, ";") + 1, "%e&nbsp;%b", &tm_departure);
		free(node_text);
		
		//Departure hour
		n_hour = findNodeNByClass(n_train_cur->node, 1, "hour");
		check(n_hour, "1st hour node (departure) not found");
		node = findNodeNByName(n_hour, 2, "p");
		getNodeText(tdoc, node, &node_text);
		strptime(node_text, "%Hh%M", &tm_departure);
		free(node_text);

		//Departure station 
		n_station = findNodeNByClass(n_train_cur->node, 1, "station");
		check(n_station, "Departure station node not found");
		node = findNodeNByName(n_station, 2, "p");
		getNodeText(tdoc, node, &node_text);
		debug("Departure station [%s]", strtrim(node_text));
		station_find_or_insert(db_hdl, strtrim(node_text), NULL, &stn_dep_id);
		debug("Departure station = [%s](id=%d)", node_text, stn_dep_id);
		check(stn_dep_id, "Couldn't find departure station id");
		free(node_text);

		//Arrival time 
		tm_arrival = tm_departure;
		n_duration = findNodeNByClass(n_train_cur->node, 1, "digital-fusion-duration_label");
		check(n_duration, "2nd hour node (departure) not found");
		getNodeText(tdoc, n_duration, &node_text);
		debug("arr time = %s", strtrim(node_text));
		sscanf(strstr(node_text, ";") + 1, "%02dh%02d", &dur_h, &dur_m);
		debug("duration = %dh%d", dur_h, dur_m);
		tm_arrival.tm_hour += dur_h;
		tm_arrival.tm_min += dur_m;
		free(node_text);

		//Arrival station 
		n_station = findNodeNByClass(n_train_cur->node, 2, "station");
		check(n_station, "Arrival station node not found");
		node = findNodeNByName(n_station, 2, "p");
		getNodeText(tdoc, node, &node_text);
		debug("Arrival station [%s]", strtrim(node_text));
		station_find_or_insert(db_hdl, strtrim(node_text), NULL, &stn_arr_id);
		debug("Arrival station = [%s](id=%d)", strtrim(node_text), stn_arr_id);
		check(stn_arr_id, "Couldn't find arrival station id");
		free(node_text);


		//transporter 
		n_transporter = findNodeNByClass(n_train_cur->node, 1, "transporteur-txt");
		check(n_transporter, "transporter node not found");
		getNodeText(tdoc, n_transporter, &node_text);
		operator = strdup(strstr(strtrim(node_text), "\n") + 1); 
		debug("transporter = %s", operator) ;
		free(node_text);

		//train number 
		n_train_nr = findNodeNByClass(n_train_cur->node, 1, "train_number");
		check(n_train_nr, "train_number node not found");
		node = findNodeNByName(n_train_nr, 2, "p");
		getNodeText(tdoc, node, &node_text);
		sscanf(node_text, "%d", &train_nr);
		debug("train_number = %d", train_nr);
		free(node_text);

		//Get all prices and save the cheapest
		res = findNodesByClass(&nodes, n_train_cur->node, "price");
		debug("Found %d prices", res);
		for(node_cur=nodes; node_cur; node_cur=node_cur->next) {
			int integer, decimal;
			float new_price;
			getNodeText(tdoc, node_cur->node, &node_text);
			res = sscanf(node_text, "%d\n.%d", &integer, &decimal);
			new_price = integer + (float)decimal/100.0;
			if (new_price < price)
				price = new_price;
			free(node_text);
		}
		freeNodeList(&nodes);

		//Ok, a good train, allocate memory
		train_new = malloc(sizeof(struct train_list_t));
		check_mem(train_new);
		train_new->next = NULL;
		train_new->train.operator = NULL;
		if(train) {
			train->next = train_new;
			train = train_new;
		} else { //first assignment
			trains_head = train = train_new;		
		}


		train->train.stn_departure = stn_dep_id;	
		train->train.stn_arrival = stn_arr_id;	
		train->train.time_departure = mktime(&tm_departure) ;	
		train->train.time_arrival = mktime(&tm_arrival) ;	
		train->train.operator = operator;	
		train->train.price = price;
		train->train.number = train_nr;
		i++;
	}
	freeNodeList(&n_trains);

goto success;
error:
	free_trains(trains_head);
	trains_head = NULL;
success:
	*ret = trains_head;
	debug("returning from parse");
	return i;

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


