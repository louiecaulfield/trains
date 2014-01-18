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
int station_get(sqlite3 *db_hdl, int id, char **name)
{
	int res = 0;
	sqlite3_stmt *sql_stmt = NULL;
	char *sql_query;

	*name = NULL;	
	asprintf(&sql_query,
		"SELECT name FROM stations WHERE stationid=%d LIMIT 1;",
		id);
	res = sqlite3_prepare_v2(db_hdl, sql_query, -1, &sql_stmt, NULL);
	debug("qry =[%s]", sql_query);
	free(sql_query);	
	check(res==SQLITE_OK, "failed to prepare sql statement (%s)", sqlite3_errmsg(db_hdl));	

	if( sqlite3_step(sql_stmt)==SQLITE_ROW ) {
		*name = strdup((const char*)sqlite3_column_text(sql_stmt, 0));
	} 

	if(!*name) {
		debug("found station [id=%d, name=%s]", id, *name);	
	}
error:
	sqlite3_finalize(sql_stmt);
	return *name?0:-1;
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
		if(name) {
			free(_name);
			_name = strdup((const char*)sqlite3_column_text(sql_stmt, 1));
		}
	} 

	if(_id) {
		debug("found station [id=%d, name=%s]", _id, _name);	
	}
error:
	sqlite3_finalize(sql_stmt);
	if(name) *name = _name;
	if(id && _id) *id = _id;
	return _id?0:-1;
}

int station_insert(sqlite3 *db_hdl, const char *name)
{
	int res;
	sqlite3_stmt *sql_stmt = NULL;
	char *sql_query;

	asprintf(&sql_query, "INSERT INTO stations (name) VALUES ('%s')", name);
	res = sqlite3_prepare_v2(db_hdl, sql_query, -1, &sql_stmt, NULL);
	debug("qry =[%s]", sql_query);
	free(sql_query);	
	check(res==SQLITE_OK, "failed to prepare sql statement (%s)", sqlite3_errmsg(db_hdl));	
	check(sqlite3_step(sql_stmt) == SQLITE_DONE,
		"Failed to insert new station: %s", sqlite3_errmsg(db_hdl));
	sqlite3_finalize(sql_stmt);
	return 0;
error:
	sqlite3_finalize(sql_stmt);
	return -1;
}
int station_find_or_insert(sqlite3 *db_hdl, const char *query, char **name, int* id)
{
	if(station_find(db_hdl, query, name, id) == 0) {
		return 0;
	}
	station_insert(db_hdl, query);
	return station_find(db_hdl, query, name, id);
}
size_t train_store(sqlite3 *db_hdl, struct train_list_t *trains)
{
	size_t n = 0;
	int res;
	struct train_list_t *train;
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

	for(train = trains; train; train = train->next) {
		sqlite3_bind_int(sql_stmt, 1, train->train.stn_departure);
		sqlite3_bind_int(sql_stmt, 2, train->train.stn_arrival);
		sqlite3_bind_int64(sql_stmt, 3, train->train.time_departure); 
		sqlite3_bind_int64(sql_stmt, 4, train->train.time_arrival); 
		sqlite3_bind_int(sql_stmt, 5, 1); //Legs 
		sqlite3_bind_double(sql_stmt, 6, train->train.price); 
		sqlite3_bind_text(sql_stmt, 7, train->train.operator, -1, NULL); 
		sqlite3_bind_int64(sql_stmt, 8,  current_time); 

		res = sqlite3_step(sql_stmt);
		check(res==SQLITE_DONE, "failed to insert row (%s)", 
			sqlite3_errmsg(db_hdl));
		n++;
		sqlite3_reset(sql_stmt);
	}
error:
	sqlite3_finalize(sql_stmt);
	return n;
}
