#include "doListen.h"

#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>

int doListen(int sockNumber, int MaxConnections) {

	int listenNumber;

	// Creacion del Log
	t_log * mySocketLog = CreacionLog("communication.log", "doListen.c");
	log_info(mySocketLog, "Comienzo el listen. Socket: %d", sockNumber);

	listenNumber = listen(sockNumber, MaxConnections);

	if (listenNumber == -1) {
		log_error(mySocketLog, "Hubo un error en el listen");
		log_error(mySocketLog, strerror(errno));
		exit(EXIT_FAILURE);
	}

	log_info(mySocketLog, "Finalizado el listen");
	log_destroy(mySocketLog);//20161205

	return sockNumber;

}
