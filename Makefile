#np_project2
CC := g++
boost_lib := /usr/local/boost_1_70_0
linker := -lboost_system -lpthread

all: console panel http

console: cgi/renderConsoleCGI.hpp cgi/renderConsoleCGI.cpp
	$(CC) -I $(boost_lib) $(linker) $^ -o console.cgi

panel: cgi/renderPanelCGI.cpp
	$(CC) $^ -o panel.cgi

http: http_server.cpp 
	$(CC) -I $(boost_lib) $(linker) $^ -o http_server

