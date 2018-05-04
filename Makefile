CC=gcc
FLAGS=-Wall -Werror -Wno-pointer-sign -lm -g

all:
	$(MAKE) clean
	$(MAKE) tcp
	$(MAKE) udp

tcp:
	$(CC) TCP_Client.c -o client_tcp $(FLAGS)
	$(CC) TCP_Server.c -o server_tcp $(FLAGS)

udp:
	$(CC) UDP_Client.c -o client_udp $(FLAGS)
	$(CC) UDP_Server.c -o server_udp $(FLAGS)

clean:
	rm client_udp server_udp client_tcp server_tcp -f
