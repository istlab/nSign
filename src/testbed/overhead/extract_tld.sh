#!/usr/bin/env bash

SIG=signatures.db

if [ ! -z $1 ]; then
	SIG=$1
fi

echo "select domainname,id from domain;" |
sqlite3 $SIG|
while read line; do 
	domain=`echo $line|cut -f1 -d'|'|sed -e 's/^/http:\/\//'|xargs -Istr ruby domain.rb str`; 
	upd=`echo $line|cut -f2 -d'|'|xargs -Istr echo update domain set tld=\"$domain\" where id=str\;`; 
	echo $upd ; 
done |
sqlite3 $SIG

