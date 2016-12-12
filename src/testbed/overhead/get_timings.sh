#!/usr/bin/env bash

echo "select distinct(tld) from domain;"|
sqlite3 signatures.db|
while read tld; do
   numscripts=`echo "select count(distinct (hash)) from signature where domainid in (select id from domain where tld='$tld');"|sqlite3 signatures.db`;
   echo "select round(avg(sig)), round(avg(db)), round(avg(stack)), round(avg(args)), round((avg(sig)+avg(db)+avg(stack)+avg(args))), round(avg(script)), round(((avg(sig)+avg(db)+avg(stack)+avg(args))/avg(script))*100) from bench where type in ('EVAL', 'JS') and hash in (select hash from signature where domainid in (select id from domain where tld='$tld'));"|
   sqlite3 signatures.db|
   xargs -Istr echo "$tld|$numscripts|"str|
   sed -e 's/\.0//g'|
   tr '|' '&'|
   sed -e 's/\&/ \& /g'|
   sed -e 's/$/ \\\% \\\\/g'
done
