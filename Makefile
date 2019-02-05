.PHONY:all
all:httpserver cgi_main

httpserver:http_server.cc http_server_main.cc 
	g++ $^ -o $@ -std=c++11 -lpthread 

cgi_main:cgi_main.cc
	g++ $^ -o $@ -std=c++11 -lpthread 

.PHONY:clean
clean:
	rm httpserver cgi_main
