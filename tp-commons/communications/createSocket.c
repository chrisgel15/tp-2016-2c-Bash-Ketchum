#include "createSocket.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <commons/log.h>
#include <commons/error.h>
#include <errno.h>

ltn_sock_addinfo* createSocket(const char * portNumber) {
	struct addrinfo hints;
	struct addrinfo *serverInfo;
	int socketNumber = 0;
	int varGetAddrInfo;
	int reuseAddrYes = 1;

	// Creacion del Log
	t_log * mySocketLog = CreacionLog("communication.log", "createSocket.c");

	memset(&hints, 0, sizeof hints); // make sure the struct is empty
	hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
	hints.ai_flags = AI_PASSIVE;    // fill in my IP for me

	if ((varGetAddrInfo = getaddrinfo(NULL, portNumber, &hints, &serverInfo))
			!= 0) {
		log_error(mySocketLog, "Error en GetAddrInfo. Puerto: %s", portNumber);
		log_error(mySocketLog, gai_strerror(varGetAddrInfo));
		log_error(mySocketLog, strerror(errno));
		exit(EXIT_FAILURE);
	}

	socketNumber = socket(serverInfo->ai_family, serverInfo->ai_socktype,
			serverInfo->ai_protocol);

	if (socketNumber == -1) {
		log_error(mySocketLog, "Hubo un error al crear el socket.");
		exit(EXIT_FAILURE);
	}

	log_info(mySocketLog, "Socket abierto: %d ", socketNumber);

	if (setsockopt(socketNumber, SOL_SOCKET, SO_REUSEADDR, &reuseAddrYes,
			sizeof(int)) == -1) {
		log_error(mySocketLog, "Hubo un error al REUTILIZAR el socket.");
		exit(EXIT_FAILURE);
	}

	log_info(mySocketLog, "Socket reutilizado. Fin createSocket.c ");

	log_destroy(mySocketLog);

	ltn_sock_addinfo* returnValue = malloc(sizeof(ltn_sock_addinfo));

	returnValue->socketNumber = socketNumber;
	returnValue->serverinfo = serverInfo;

	return returnValue;
}

