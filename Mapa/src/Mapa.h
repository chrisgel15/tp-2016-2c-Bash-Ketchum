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
#include <stdbool.h>
#include "Estructuras.h"
#include "Pokenest.h"

//Varibles para el Log del Programa
#define log_nombre "mapa.log"
#define programa_nombre "Mapa.c"
#define POSICION_INICIAL_X 0
#define POSICION_INICIAL_Y 0

/********* FUNCIONES PARA EL MANEJO DE ESTRUCTURAS DE ESTADOS *********/
void agregar_entrenador_a_listos(t_entrenador *entrenador);
t_entrenador *remover_entrenador_listo_por_RR();
t_entrenador *remover_entrenador_listo_por_SRDF();
void agregar_entrenador_a_bloqueados(t_entrenador *);
void agregar_entrenador_a_ejecutando(t_entrenador *);
void remover_entrenador_de_ejecutando(int);


/********* FUNCIONES DE INICIALIZACION *********/
void inicializar_estructuras();

/********* FUNCIONES PARA RECIBIR PETICIONES DE LOS ENTRENADORES *********/
void atender_entrenador(int fd_entrenador, int codigo_instruccion);
void recibir_nuevo_entrenador(int fd);
void recibir_mensaje_entrenador(int fd);
void despedir_entrenador(int fd_entrenador, int codigo_instruccion);

t_entrenador *buscar_entrenador(int fd);

void posicionar_entrenador_en_mapa(t_datos_mapa*,char* );
void mover_entrenador_hacia_recurso(t_datos_mapa*,char*);

void entregar_pokemon(t_entrenador*);
void entregar_medalla(t_entrenador *entrenador, char* nombre_mapa);
void enviar_posicion_pokenest(int fd, t_mensajes *mensajes);
void avanzar_hacia_pokenest(t_entrenador *entrenador, t_mensajes *mensajes);

/********* FUNCION ENCARGADA DEL MANEJO DE LAS SYSTEM CALLS*********/
void system_call_catch(int signal);


void administrar_turnos();
void atender_Viaje_Entrenador(t_entrenador* entrenador, bool es_algoritmo_rr);
void administrar_bloqueados();
/*** Funcion para obtener el algoritmo despues de ser actualizado por la señal ***/
char* algoritmo_actual();

/*** Funcion para obtener el quantum despues de ser actualizado por la señal ***/
int quantum_actual();

/* Actualizacion de valores desde el Archivo de comfiguracion */
void set_algoritmoActual();
void set_quantum();
void set_retardo();
void set_interbloqueo();

#endif /* MAPA_H_ */
