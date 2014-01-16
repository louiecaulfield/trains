PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS countries(
	countryid	INTEGER PRIMARY KEY,
	name		TEXT UNIQUE NOT NULL,
	iso_code	TEXT UNIQUE NOT NULL
	);

CREATE TABLE IF NOT EXISTS cities(
	cityid		INTEGER PRIMARY KEY,
	name		TEXT UNIQUE NOT NULL,
	lat		REAL,
	long		REAL,
	country		REFERENCES countries(countryid)	
	);

CREATE TABLE IF NOT EXISTS stations(
	stationid	INTEGER PRIMARY KEY,
	name 		TEXT UNIQUE NOT NULL,
	lat 		REAL,
	lon 		REAL,
	address 	TEXT,
	city		REFERENCES cities(cityid)
	);
CREATE TABLE IF NOT EXISTS operators(
	operatorid	INTEGER PRIMARY KEY,
	name		TEXT
	);

CREATE TABLE IF NOT EXISTS trains(
	stn_dep 	REFERENCES stations(stationid),
	stn_arr 	REFERENCES stations(stationid),
	time_dep 	INTEGER,
	time_arr 	INTEGER,
	legs		INTEGER,
	price		REAL,
	operator	REFERENCES operators(operatorid)
	);
	
