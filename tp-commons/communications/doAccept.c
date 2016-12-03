#include "doBind.h"

#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>

int doAccept(int socketNumber) {

	int returnSocket;
	struct sockaddr_storage their_addr;
	socklen_t addr_size;

	// Creacion del Log
	t_log * mySocketLog = CreacionLog("communication.log", "doAccept.c");

	log_info(mySocketLog, "Comienzo el Accept. Socket: %d", socketNumber);

	addr_size = sizeof their_addr;
	returnSocket = accept(socketNumber, (struct sockaddr *)&their_addr, &addr_size);

	if (returnSocket == -1) {
		log_error(mySocketLog, "Hubo un error en el Accept");
		log_error(mySocketLog, strerror(errno));
		exit(EXIT_FAILURE);
	}

	log_info(mySocketLog, "Finalizado el Accept");

	log_destroy(mySocketLog);

	return returnSocket;

}
