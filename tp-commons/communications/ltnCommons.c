#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/string.h>
#include "ltnCommons.h"
#include "instructionCodes.h"


int enviarInt(int clientFd, int mensaje) {
	int net_mensaje = htonl(mensaje);
	return send(clientFd, (char *) &net_mensaje, sizeof(int), 0);
}

int enviarMensaje(int clienteFd, char* mensaje) {
	return send(clienteFd, mensaje, strlen(mensaje), 0);
}

int enviarBytes(int clienteFd, void * datos, int tamanio) {
	return send(clienteFd, datos, tamanio, 0);
}

int recibirBytes(int fd, void * buf, int tamanio, t_log * myLog) {
	return recibirMensaje(fd, (char *)buf, tamanio, myLog);
}

int recibirMensaje(int fd, char * buf, int tamanio, t_log * myLog) {
	int bytes = 0;
	bytes = recv(fd, (void *) buf, sizeof(char) * tamanio, 0);
	if (bytes <= 0) {
		log_error(myLog, "Error al recibir mensaje.");
		return NO_SE_RECIBIERON_DATOS;
	} else {
		log_trace(myLog, "Recibi %d bytes desde el Socket.", bytes);
		buf[bytes] = '\0';
		return bytes;
	}
}

int recibirInt(int fd, int * result, t_log * myLog) {
	int ret;
	*result = recv(fd, &ret, sizeof(int), 0);
	if (*result <= 0)
		log_warning(myLog,
				"Error al Recibir Entero (numero negativo o cero). Posible Desconexion");
	return ntohl(ret);
}

t_log * CreacionLog(char * logName, char * programName) {
	// Creacion del Log
	char logFilePath[100];
	strcpy(logFilePath, "./config/");
	strcat(logFilePath, logName);

	t_log_level errorLevel = LOG_LEVEL_TRACE;

	return log_create(logFilePath, programName, true, errorLevel);
}

t_log * CreacionLogWithLevel(char * logName, char * programName, char * log_level) {
	// Creacion del Log
	char logFilePath[100];
	strcpy(logFilePath, "./config/");
	strcat(logFilePath, logName);

	t_log_level errorLevel = log_level_from_string(log_level);

	return log_create(logFilePath, programName, true, errorLevel);
}


t_config* creacion_config() {
	char* configFilePath = "./config/configFile.txt";
	t_config* myConfig;
	myConfig = config_create(configFilePath);
	return myConfig;
}

