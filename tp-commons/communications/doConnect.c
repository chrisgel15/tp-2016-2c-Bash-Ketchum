#include "doBind.h"

#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>

int doConnect(ltn_sock_addinfo * sockAddrInfo) {

	char * logFilePath = "./config/log.log";
	int connectNumber;

	// Creacion del Log
	t_log * mySocketLog = CreacionLog("communication.log", "doConnect.c");

	log_info(mySocketLog, "Comienzo el connect. Socket: %d",sockAddrInfo->socketNumber);

	connectNumber = connect(sockAddrInfo->socketNumber, sockAddrInfo->serverinfo->ai_addr, sockAddrInfo->serverinfo->ai_addrlen);

	if (connectNumber == -1) {
		log_error(mySocketLog, "Hubo un error en el connect");
		log_error(mySocketLog, strerror(errno));
		exit(EXIT_FAILURE);
	}

	log_info(mySocketLog, "Finalizado el connect");

	int returnSock = sockAddrInfo->socketNumber;

	freeaddrinfo(sockAddrInfo->serverinfo); // free the linked-list
	free(sockAddrInfo);

	return returnSock;

}
