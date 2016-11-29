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
#include <stdbool.h>
#include "Gps.h"
#include <time.h>
#include <commons/collections/list.h>
#include <dirent.h>
#include <string.h>
#include <commons/string.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

//Varibles para el Log del Programa
#define log_nombre "entrenador.log"
#define programa_nombre "Entrenador.c"

#define CONECTARSE_MAPA 999

typedef struct{
	char* nombre;
	int vidas;
	int reintentos;
} t_entrenador;

void system_call_catch(int signal);

//Funcion para primer Check Point que se va a encargar de recibir mensajes del Servidor
void recibir_mensajes();

void solicitar_posicion_pokenest(char *pokemon);

int conectar_mapa(char*, char*);

void capturar_pokemon(char*, t_list*, int, int*);

int avanzar_hacia_pokenest();

bool objetivoCumplido(int posHojaDeViaje, int posPokenest);

void terminarObjetivo();

void convertirseEnMaestroPokemon(time_t tiempo_total_Viaje, time_t tiempo_total_bloqueado, int );

void recorrer_hojaDeViaje(int posHojaDeViaje);

void reiniciar_Hoja_De_Viaje(int );

void init_datos_entrenador(void);

void handshake();

void borrar_medallas();

void borrar_pokemon();

void borrar_pokemons_de_un_mapa(t_list * pokemons);

void liberar_recursos();

#endif /* ENTRENADOR_H_ */
