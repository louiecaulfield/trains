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

int station_get_name(sqlite3 *db_hdl, const char *query, char **name)
{
	int res, i;
	sqlite3_stmt *sql_stmt = NULL;
	char *sql_query;

	//FIXME: return all results found and provide means for user to select?
	asprintf(&sql_query,
		"SELECT name FROM stations WHERE name LIKE '%s%%' LIMIT 1;",
		query);
	debug("Query = [%s]", sql_query);
	res = sqlite3_prepare_v2(db_hdl, sql_query, -1, &sql_stmt, NULL);
	free(sql_query);	
	check(res==SQLITE_OK, "failed to prepare sql statement");	

	*name = NULL;
	while(1) {
		res = sqlite3_step(sql_stmt);
		if(res != SQLITE_ROW) {
			log_info("Failed to find a result: %s",
				sqlite3_errmsg(db_hdl));
			break;	
		}
		for(i = 0; i < sqlite3_column_count(sql_stmt); i++) {
			debug("col type %d = %d", i, sqlite3_column_type(sql_stmt, i));
			switch(sqlite3_column_type(sql_stmt, i)) {
				case SQLITE_TEXT:
				free(*name);
				*name = strdup(sqlite3_column_text(sql_stmt, i));
				debug("Name is now %s", *name);
				break;
				default:
				debug("not using column %d", i);
			}
		}		
	} 
	
error:
	sqlite3_finalize(sql_stmt);
	return *name?0:-1;
}

int train_store(sqlite3 *db_hdl, struct train_t *trains, size_t ntrains)
{
	return 0;
}
