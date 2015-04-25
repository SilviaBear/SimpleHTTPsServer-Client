all:http_client http_server https_server

http_client: connect_tls.c connect_tls.h http_client.c 
	gcc -o http_client http_client.c connect_tls.c -lpolarssl
http_server: http_server.c
	gcc -o http_server http_server.c
https_server: connect_tls.c connect_tls.h https_server.c
	gcc -o https_server https_server.c connect_tls.c -lpolarssl