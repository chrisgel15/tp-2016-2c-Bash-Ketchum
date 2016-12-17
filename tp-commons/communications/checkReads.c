#include "checkReads.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <commons/txt.h>

ltn_fd_sets * checkReads(ltn_fd_sets * fdSets, void (*funcProcessInstructionCode)(int, int), void(* funcDesconectados)(int), t_log * myLog) {
	int i;
	struct sockaddr_storage remoteAddr;
	socklen_t addrlen;
	int nuevaConexion;
	int bytesRecibidos;
	int * result = malloc(sizeof(int));
	//int * result;

	//Run through the existing connections looking for data to read
	for (i = 0; i <= (fdSets->maxFileDescriptorNumber); i++) {
		if (FD_ISSET(i, &(fdSets->readFileDescriptorSet))) { // we got one!!
			if (i == (fdSets->listenerSocket)) {
				// Se acepta la nueva conexion, luego se hara el HANDSHAKE
				addrlen = sizeof remoteAddr;
				nuevaConexion = accept(fdSets->listenerSocket, (struct sockaddr *) &remoteAddr, &addrlen);
				if (nuevaConexion == -1) {
					perror("Server: Se produjo un error al aceptar la nueva conexión.\n");
				} else {
					FD_SET(nuevaConexion, &(fdSets->masterSet)); // add to master set
					if (nuevaConexion > fdSets->maxFileDescriptorNumber) { // keep track of the max
						fdSets->maxFileDescriptorNumber = nuevaConexion;
					}
					log_info(myLog,"Server: Hay una nueva conexión en el Socket %d.",nuevaConexion);
				}
			} else {
				// handle data from a client
				//result = malloc(sizeof(int));
				bytesRecibidos = recibirInt(i, result, myLog);
				if (*result <= 0) {
					// got error or connection closed by client
					if (*result == 0){ // connection closed
						log_error(myLog, "Select Server: Se desconecto el cliente %d",i);
						(*funcDesconectados)(i);
					} else {
						log_error(myLog,"Error en RECV de nuevos datos. Socket %d", i);
						perror("Error en RECV");
					}
					close(i); // bye!
					FD_CLR(i, &(fdSets->masterSet)); // remove from master set
				} else {
					log_info(myLog, "Se recibio del socket %d. Mensaje: %d", i, bytesRecibidos);
					log_info(myLog, "Cantidad de bytes: %d", bytesRecibidos);
					(*funcProcessInstructionCode)(i, bytesRecibidos);
					// we got some data from a client
				}
			}
			free(result);// END handle data from client
		} // END got new incoming connection
	} // END looping through file descriptors
	return fdSets;
}
