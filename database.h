#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include "trains.h"

//! Open sqlite3 database and return handle
/*
	\param db_hdl Pointer to *db_hdl in which the created db handle will be stored
	\param db_file String containing the database filename
	\return 0 for success, -1 for failure
*/
int database_init(sqlite3 **db_hdl, const char *dbfile);
//! Close sqlite3 database
/*
	\param db_hdl database handle to close
*/
void database_cleanup(sqlite3 *db_hdl);

//! Find a station in the sqlite3 table by name
/*
	\param db_hdl The sqlite3 handle
	\param query The station name you're looking for, will try to find stations LIKE 'query%'
	\param name [out]The string of the found station name, can be NULL if you don't care
	\param id [out]The id of the found station, can be NULL if you don't care
	\result 0 if station found, -1 if no station found
*/
int station_find(sqlite3 *db_hdl, const char *query, char **name, int* id);

//! Get a station from the sqlite3 table by id
/*
	\param db_hdl The sqlite3 handle
	\param id The id of the row
	\param name [out]The string of the found station name, can be NULL if you don't care
	\result 0 if station found, -1 if no station found
*/
int station_get(sqlite3 *db_hdl, int id, char **name);

//! Get a station from the sqlite3 table by id
/*
	\param db_hdl The sqlite3 handle
	\param name The string of the new station name
	\result 0 if station insterted, -1 if failed 
*/
int station_insert(sqlite3 *db_hdl, const char *name);

//! Try to find a station by name and insert if not found
/*
	\param db_hdl The sqlite3 handle
	\param query String containing the station name you're looking for
	\param name [out]Station name found in the database, can be NULL
	\param id [out]Station id found in database, can be NULL	
	\return 0 if station inserted and then found, -1 if failure
*/
int station_find_or_insert(sqlite3 *db_hdl, const char *query, char **name, int* id);

//! Store linked list of trains in database
/*
	\param db_hdl The sqlite3 handle
	\param trains The head node of the linked list you're storing
	\return number of nodes stored
*/
size_t train_store(sqlite3 *db_hdl, struct train_list_t *trains);

#endif
