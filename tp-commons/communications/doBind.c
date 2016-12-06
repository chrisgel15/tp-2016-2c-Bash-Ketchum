#include "doBind.h"

#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>

int doBind(ltn_sock_addinfo * sockAddrInfo) {

	int bindNumber;

	// Creacion del Log
	t_log * mySocketLog = CreacionLog("communication.log", "doBind.c");

	log_info(mySocketLog, "Comienzo el bind. Socket: %d", sockAddrInfo->socketNumber);

	bindNumber = bind(sockAddrInfo->socketNumber,
			sockAddrInfo->serverinfo->ai_addr,
			sockAddrInfo->serverinfo->ai_addrlen);

	if (bindNumber == -1) {
		log_error(mySocketLog, "Hubo un error en el bind");
		log_error(mySocketLog, strerror(errno));
		exit(EXIT_FAILURE);
	}

	log_info(mySocketLog, "Finalizado el bind");

	int returnSock = sockAddrInfo->socketNumber;
	//int returnSock = malloc(sizeof(ltn_socket_number));
	//returnSock->socketNumber = sockAddrInfo->sockNum->socketNumber;

	freeaddrinfo(sockAddrInfo->serverinfo); // free the linked-list
	free(sockAddrInfo);
	log_destroy(mySocketLog);//20161205

	return returnSock;

}
