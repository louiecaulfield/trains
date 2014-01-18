#!/bin/sh
DB_FILE="test.db"
if [ -e $DB_FILE ]; then
	echo database $DB_FILE already exists!
	echo aborting
	exit -1
fi

echo Setting up database
sqlite3 $DB_FILE < setup_database.sql

#echo Filling database with SNCF cities
#./sncf_stations_to_db $DB_FILE
