#!/bin/bash

SCRAPER=./train_scraper
CONNECTIONS=connections.txt
DBFILE=test.db

rm $DBFILE 
./setup.sh

while read -r line
do
	STATIONS=$line
	ARGS=" -d $DBFILE"
#scrape A->B
	ARGS+=`echo $line | awk '{ print " -f " $1 " -t " $2}'`
	echo $SCRAPER $ARGS
	$SCRAPER $ARGS
#scrape B->A
	ARGS+=`echo $line | awk '{ print " -f " $2 " -t " $1}'`
	echo $SCRAPER $ARGS
	$SCRAPER $ARGS

done < "$CONNECTIONS"
