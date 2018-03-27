ifeq ($(OS),Windows_NT)
    suffix = dll
else
    suffix = so
endif

installpath = /usr/local/lib/deadbeef

all:
	gcc -std=c99 -shared -g -O0 -fPIC -I /usr/local/include -Wall -o stats.$(suffix) values_default.c values_playlist.c stats.c gen_html.c index1.txt.c


install:
	cp stats.$(suffix) $(installpath)
