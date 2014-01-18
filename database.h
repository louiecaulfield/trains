#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include "trains.h"

int database_init(sqlite3 **db_hdl, const char *dbfile);
void database_cleanup(sqlite3 *db_hdl);

int station_find(sqlite3 *db_hdl, const char *query, char **name, int* id);
int station_insert(sqlite3 *db_hdl, const char *name);
int station_find_or_insert(sqlite3 *db_hdl, const char *query, char **name, int* id);

size_t train_store(sqlite3 *db_hdl, struct train_list_t *trains);

#endif
