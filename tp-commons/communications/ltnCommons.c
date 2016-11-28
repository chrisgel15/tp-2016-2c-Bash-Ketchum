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
		buf[bytes] = '\0';
		log_trace(myLog, "Recibi %d bytes desde el Socket. Mensaje %s", bytes, buf);
		return bytes;
	}
}

int recibirInt(int fd, int * result, t_log * myLog) {
	int ret;
	*result = recv(fd, &ret, sizeof(int), 0);
	if (*result <= 0)
		log_warning(myLog,
				"Warning al Recibir Entero (numero negativo o cero). Posible Desconexion");
	else
		log_trace(myLog, "Recibi el valor %d, del cliente %d ", ntohl(ret), fd);
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

t_config* creacion_config_with_path(char * path) {
	char* configFilePath = path;
	t_config* myConfig;
	myConfig = config_create(configFilePath);
	return myConfig;
}

void * myMalloc(size_t size, t_log * myLog)
{
	void * returnValue = malloc(size);
	memset(returnValue, 0, size);
	if (returnValue != NULL)
	{
		log_trace(myLog, "Pude alocar correctamente %d bytes en la direccion: %04x", size, returnValue);
	}
	else
	{
		log_error(myLog, "Error al intentar alocar %d bytes", size);
	}

	return returnValue;
}

void * myMalloc_int(char * nombreDeVariable, t_log * myLog)
{
	log_trace(myLog, "Intentando alocar %d bytes para la variable %s", sizeof(int), nombreDeVariable);
	return myMalloc(sizeof(int), myLog);
}

void * myMalloc_char(int tamanio, char * nombreDeVariable, t_log * myLog)
{
	log_trace(myLog, "Intentando alocar %d bytes para la variable %s", sizeof(char)*tamanio, nombreDeVariable);
	return myMalloc(sizeof(char)*tamanio, myLog);
}

void * myMalloc_ltn_sock_addinfo(char * nombreDeVariable, t_log * myLog)
{
	log_trace(myLog, "Intentando alocar %d bytes para la variable %s", sizeof(ltn_sock_addinfo), nombreDeVariable);
	return myMalloc(sizeof(ltn_sock_addinfo), myLog);
}

void myFree(void * ptr, char * nombreDeVariable, t_log * myLog)
{
	log_trace(myLog, "Liberando la variable %s en la direccion %04x", nombreDeVariable, ptr);
	free(ptr);
}

void myFreeSplitPath(char ** arr, char * nombreDeVariable, t_log * myLog)
{
	int index = 0;
	while(*(arr + index) != NULL)
	{
		log_trace(myLog, "Liberando SplitPath arr[%d]", index);
		myFree(*(arr + index), "SplitPath - arr[index]", myLog);
		index++;
	}
	myFree(arr,"SplitPath - arr", myLog);
}



