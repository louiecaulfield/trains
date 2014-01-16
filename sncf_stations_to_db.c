#include <stdlib.h>
#include <sqlite3.h>
#include "include/dbg.h"
#include "sncf_stations.h"

int main(int argc, char *argv[])
{
	int res, i, nstations;
	sqlite3 *db_hdl = NULL;
	sqlite3_stmt *sql_stmt = NULL;
	char *sql_query;

	if(argc != 2) {
		log_err("Usage: %s <db>", argv[0]);
		return -1;
	}

	//Open database
	res = sqlite3_open(argv[1], &db_hdl);
	check(res==SQLITE_OK, "failed to open database");
	
	//Build query
	asprintf(&sql_query, "INSERT into stations (name, lat, lon, address)"
			"VALUES(?, ?, 0, \"Dude I dunno\")");
	check(sql_query, "failed to assign sql_query string");

	res = sqlite3_prepare_v2(db_hdl, sql_query, -1, &sql_stmt, NULL);
	free(sql_query);
	check(res==SQLITE_OK, "failed to prepare sql statement");

	//iterate over stations
	nstations = sizeof(sncf_stations)/sizeof(sncf_stations[0]);
	debug("gonna iterate %d times", nstations);
	for(i = 0; i < nstations; i++) {
		res = sqlite3_bind_text(sql_stmt, 1, sncf_stations[i], -1, NULL);
		check(res==SQLITE_OK, "failed to bind station name to statement");
		sqlite3_bind_int(sql_stmt, 2, i);
		check(res==SQLITE_OK, "failed to bind latitude to statement");
		res = sqlite3_step(sql_stmt);
		if(res != SQLITE_DONE)
			log_info("Query execution returned [%s]", sqlite3_errmsg(db_hdl));
		sqlite3_reset(sql_stmt);
	}

error:
	//cleanup
	res = sqlite3_finalize(sql_stmt);
	check(res==SQLITE_OK, "failed to finalize statement");
	res = sqlite3_close(db_hdl);
	check(res==SQLITE_OK, "failed to close database");

	return 0;
}
