#!/bin/sh

if command -v db2html 2>&1 >/dev/null; then
	db2html -u Handbook.xml
	mv Handbook/Handbook.html index.html
else
	echo "Install db2html to create index.html!"
	touch index.html
fi
