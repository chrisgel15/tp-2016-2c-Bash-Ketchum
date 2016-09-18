#ifndef ENTRENADOR_H_
#define ENTRENADOR_H_

#include <stdio.h>
#include <stdlib.h>
#include <communications/ltnCommons.h>
#include <communications/instructionCodes.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <commons/config.h>
#include <commons/log.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <metadata/entrenador.h>

//Varibles para el Log del Programa
#define log_nombre "entrenador.log"
#define programa_nombre "Entrenador.c"

void system_call_catch(int signal);

//Funcion para primer Check Point que se va a encargar de recibir mensajes del Servidor
void recibir_mensajes();

void solicitar_posicion_pokenest(t_config* ,char *,int );

int conectar_mapa(char*, char*);

void capturar_pokemon(char*);
#endif /* ENTRENADOR_H_ */
