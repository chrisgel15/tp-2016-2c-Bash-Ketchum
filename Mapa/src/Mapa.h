#ifndef MAPA_H_
#define MAPA_H_

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <communications/ltnCommons.h>
#include <communications/instructionCodes.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/collections/queue.h>
#include <pthread.h>
#include <commons/string.h>
#include "Interfaz_Grafica.h"
#include <string.h>

//Varibles para el Log del Programa
#define log_nombre "mapa.log"
#define programa_nombre "Mapa.c"

//Estructuras
typedef struct{
	char *nombre;
	int fd;
	char caracter; //Caracter que va a representar al Entrendor en el Mapa
	t_posicion* posicion;
} t_entrenador;

/********* FUNCIONES DE INICIALIZACION *********/
void inicializar_estructuras();

/********* FUNCIONES PARA RECIBIR PETICIONES DE LOS ENTRENADORES *********/
void atender_entrenador(int fd_entrenador, int codigo_instruccion);
void recibir_nuevo_entrenador(int fd);
void recibir_mensaje_entrenador(int fd);
void despedir_entrenador(int fd_entrenador, int codigo_instruccion);

t_entrenador *buscar_entrenador(int fd);

#endif /* MAPA_H_ */
