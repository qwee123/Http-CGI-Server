#np_project2
CC := g++
shell_dir := ./shell
socket_dir := ./passiveSocket
socket := $(socket_dir)/passiveSocket.h $(socket_dir)/passiveSocket.cpp
np_simple_shell := $(shell_dir)/np_shell.cpp $(shell_dir)/np_shell.h $(shell_dir)/shellCmd.h $(shell_dir)/shellCmd.cpp
np_single_proc := $(shell_dir)/singleProcShell.cpp $(shell_dir)/singleProcShell.h $(shell_dir)/shellCmd.h $(shell_dir)/shellCmd.cpp 
np_multi_proc := $(shell_dir)/multiProcShell.cpp $(shell_dir)/multiProcShell.h $(shell_dir)/shellCmd.h $(shell_dir)/shellCmd.cpp

all: simple single multi

simple: np_simple.cpp $(socket) $(np_simple_shell)
	$(CC) $^ -o np_simple

single: np_single_proc.cpp $(socket) $(np_single_proc)
	$(CC) $^ -o np_single_proc

multi: np_multi_proc.cpp $(socket) $(np_multi_proc)
	$(CC) $^ -o np_multi_proc

clean:
	rm -f npshell
