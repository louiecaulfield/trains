#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>

#include "database.h"
#include "trains.h"
#include "include/dbg.h"

int database_init(sqlite3 **db_hdl, const char *dbfile)
{
	int res;
	debug("trying to open db %s", dbfile);
	res = sqlite3_open(dbfile, db_hdl);
	check(res==SQLITE_OK, "failed to open database");
	return 0;
error:
	return -1;	
}

void database_cleanup(sqlite3 *db_hdl)
{
	sqlite3_close(db_hdl);
}

int station_find(sqlite3 *db_hdl, const char *query, char **name, int* id)
{
	int res, _id = 0;
	sqlite3_stmt *sql_stmt = NULL;
	char *sql_query, *_name = NULL;

	//FIXME: return all results found and provide means for user to select?
	asprintf(&sql_query,
		"SELECT stationid, name FROM stations WHERE name LIKE '%s%%' LIMIT 1;",
		query);
	res = sqlite3_prepare_v2(db_hdl, sql_query, -1, &sql_stmt, NULL);
	debug("qry =[%s]", sql_query);
	free(sql_query);	
	check(res==SQLITE_OK, "failed to prepare sql statement (%s)", sqlite3_errmsg(db_hdl));	

	while( sqlite3_step(sql_stmt)==SQLITE_ROW ) {
		_id = sqlite3_column_int(sql_stmt, 0);
		free(_name);
		_name = strdup((const char*)sqlite3_column_text(sql_stmt, 1));
	} 
	debug("found station [id=%d, name=%s]", _id, _name);	
error:
	sqlite3_finalize(sql_stmt);
	if(name && _name) *name = _name;
	if(id && _id) *id = _id;
	return _name?0:-1;
}

size_t train_store(sqlite3 *db_hdl, struct train_t *trains, size_t ntrains)
{
	int res;
	size_t i = 0;
	sqlite3_stmt *sql_stmt = NULL;
	char *sql_query;
	time_t current_time;

	current_time = time(NULL);
	asprintf(&sql_query,
		"INSERT INTO trains "
		"(stn_dep, stn_arr, time_dep, time_arr, legs, price, operator, time_query) "
		"VALUES(?, ?, ?, ?, ?, ?, ?, ?)");
		
	res = sqlite3_prepare_v2(db_hdl, sql_query, -1, &sql_stmt, NULL);
	free(sql_query);	
	check(res==SQLITE_OK, "failed to prepare sql statement: %s", 
			sqlite3_errmsg(db_hdl));	

	for(i = 0; i < ntrains; i++) {
		sqlite3_bind_int(sql_stmt, 1, trains[i].stn_departure);
		sqlite3_bind_int(sql_stmt, 2, trains[i].stn_arrival);
		sqlite3_bind_int64(sql_stmt, 3, trains[i].time_departure); 
		sqlite3_bind_int64(sql_stmt, 4, trains[i].time_arrival); 
		sqlite3_bind_int(sql_stmt, 5, 1); //Legs 
		sqlite3_bind_double(sql_stmt, 6, trains[i].price); 
		sqlite3_bind_text(sql_stmt, 7, trains[i].operator, -1, NULL); 
		sqlite3_bind_int64(sql_stmt, 8,  current_time); 

		res = sqlite3_step(sql_stmt);
		check(res==SQLITE_DONE, "failed to insert row (%s)", 
			sqlite3_errmsg(db_hdl));
		sqlite3_reset(sql_stmt);
	}
error:
	sqlite3_finalize(sql_stmt);
	return i;
}
