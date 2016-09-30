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
#include <semaphore.h>
#include <commons/string.h>
#include "Interfaz_Grafica.h"
#include <string.h>
#include <signal.h>

//Varibles para el Log del Programa
#define log_nombre "mapa.log"
#define programa_nombre "Mapa.c"
#define POSICION_INICIAL_X 0
#define POSICION_INICIAL_Y 0

//Estructuras
typedef struct{
	char *nombre;
	int fd;
	char caracter; //Caracter que va a representar al Entrendor en el Mapa
	t_posicion* posicion;
	char pokemon_bloqueado;
} t_entrenador;

typedef struct{
	t_list* items;
	t_entrenador* entrenador;
}t_datos_mapa;

typedef struct{
	char *nombre;
	char caracter; //Caracter que va a representar al Entrendor en el Mapa
	t_posicion* posicion;
	int cantPokemons; //registrar recursos
} t_pokenest;

/********* FUNCIONES PARA EL MANEJO DE ESTRUCTURAS DE ESTADOS *********/
void agregar_entrenador_a_listos(t_entrenador *entrenador);
t_entrenador *remover_entrenador_listo_por_RR();
t_entrenador *remover_entrenador_listo_por_SRDF();
void agregar_entrenador_a_bloqueados(t_entrenador *entrenador);

/********* FUNCIONES DE INICIALIZACION *********/
void inicializar_estructuras();

/********* FUNCIONES PARA RECIBIR PETICIONES DE LOS ENTRENADORES *********/
void atender_entrenador(int fd_entrenador, int codigo_instruccion);
void recibir_nuevo_entrenador(int fd);
void recibir_mensaje_entrenador(int fd);
void despedir_entrenador(int fd_entrenador, int codigo_instruccion);

t_entrenador *buscar_entrenador(int fd);

void posicionar_entrenador_en_mapa(t_datos_mapa*);
void mover_entrenador_hacia_recurso(t_datos_mapa*);

void entregar_pokemon(int );
void entregar_medalla();
void enviar_posicion_pokenest(int fd);


/********* FUNCION ENCARGADA DEL MANEJO DE LAS SYSTEM CALLS*********/
void system_call_catch(int signal);


void administrar_turnos();
void atender_Viaje_Entrenador(t_entrenador*);
/*** Funcion para obtener el algoritmo despues de ser actualizado por la señal ***/
char* algoritmo_actual();

/*** Funcion para obtener el quantum despues de ser actualizado por la señal ***/
int quantum_actual();

#endif /* MAPA_H_ */
