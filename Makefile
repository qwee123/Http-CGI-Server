#np_project2
CC := g++
shell_dir := ./shell
socket_dir := ./passiveSocket
socket := $(socket_dir)/passiveSocket.h $(socket_dir)/passiveSocket.cpp
np_simple_shell := $(shell_dir)/np_shell.cpp $(shell_dir)/np_shell.h $(shell_dir)/shellCmd.h $(shell_dir)/shellCmd.cpp
np_single_proc := $(shell_dir)/singleProcShell.cpp $(shell_dir)/singleProcShell.h $(shell_dir)/shellCmd.h $(shell_dir)/shellCmd.cpp 
np_multi_proc := $(shell_dir)/multiProcShell.cpp $(shell_dir)/multiProcShell.h $(shell_dir)/shellCmd.h $(shell_dir)/shellCmd.cpp
boost_lib := /usr/local/boost_1_70_0
linker := -lboost_system -lpthread

all: console panel http

simple: np_simple.cpp $(socket) $(np_simple_shell)
	$(CC) $^ -o np_simple

single: np_single_proc.cpp $(socket) $(np_single_proc)
	$(CC) $^ -o np_single_proc

multi: np_multi_proc.cpp $(socket) $(np_multi_proc)
	$(CC) $^ -o np_multi_proc

console: cgi/renderConsoleCGI.hpp cgi/renderConsoleCGI.cpp
	$(CC) -I $(boost_lib) $(linker) $^ -o console.cgi

panel: cgi/renderPanelCGI.cpp
	$(CC) $^ -o panel.cgi

http: http_server.cpp 
	$(CC) -I $(boost_lib) $(linker) $^ -o http_server

clean:
	rm -f npshell
