#include "createClientSocket.h"

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

ltn_sock_addinfo* createClientSocket(const char * serverIP, const char * portNumber) {
	struct addrinfo hints;
	struct addrinfo *serverInfo;
	int socketNumber = 0;
	int varGetAddrInfo;
	int reuseAddrYes = 1;

	// Creacion del Log
	t_log * mySocketLog = CreacionLog("communication.log", "createClientSocket.c");

	memset(&hints, 0, sizeof hints); // make sure the struct is empty
	hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM; // TCP stream sockets

	if ((varGetAddrInfo = getaddrinfo(serverIP, portNumber, &hints, &serverInfo))
			!= 0) {
		log_error(mySocketLog, "Error en GetAddrInfo");
		log_error(mySocketLog, gai_strerror(varGetAddrInfo));
		log_error(mySocketLog, strerror(errno));
		exit(EXIT_FAILURE);
	}

	socketNumber = socket(serverInfo->ai_family, serverInfo->ai_socktype,
			serverInfo->ai_protocol);

	if (socketNumber == -1) {
		log_error(mySocketLog, "Hubo un error al crear el socket cliente.");
		exit(EXIT_FAILURE);
	}

	log_info(mySocketLog, "Socket cliente abierto: %d ",socketNumber );

	if (setsockopt(socketNumber, SOL_SOCKET, SO_REUSEADDR, &reuseAddrYes,
			sizeof(int)) == -1) {
		log_error(mySocketLog, "Hubo un error al REUTILIZAR el socket cliente.");
		exit(EXIT_FAILURE);
	}

	log_info(mySocketLog, "Socket cliente reutilizado. Fin createClientSocket.c");

	log_destroy(mySocketLog);

	ltn_sock_addinfo* returnValue = malloc(sizeof(ltn_sock_addinfo));

	returnValue->socketNumber = socketNumber;
	returnValue->serverinfo = serverInfo;

	return returnValue;
}

