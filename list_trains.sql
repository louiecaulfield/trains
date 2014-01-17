SELECT 
	d.name AS stn_departure, 
	a.name AS stn_arrival, 
	datetime(time_dep, 'unixepoch') AS time_departure, 
	datetime(time_arr, 'unixepoch') AS time_arrival, 
	price AS price, 
	operator AS operator, 
	datetime(time_query, 'unixepoch') AS queried 
FROM trains AS t 
	JOIN stations as d on t.stn_dep=d.stationid 
	JOIN stations as a on t.stn_arr=a.stationid;

