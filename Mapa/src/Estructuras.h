/*
 * Estructuras.h
 *
 *  Created on: 24/10/2016
 *      Author: utnso
 */

#ifndef ESTRUCTURAS_H_
#define ESTRUCTURAS_H_

#include <commons/collections/queue.h>
#include <commons/collections/list.h>

int cantidad_pokenest;

typedef struct{
	int x;
	int y;
} t_posicion;

typedef struct {
	int id;
	char *nombre;
	int fd;
	char caracter; //Caracter que va a representar al Entrendor en el Mapa
	t_posicion* posicion;
	char *pokemon_bloqueado;
	int pokenx;		//posicion de la pokenest en el eje x
	int pokeny;		//posicion de la pokenest en el eje y
	t_list* pokemons; //En esta Lista se Encuentran los Pokemons que va atrapando el Entrenador
} t_entrenador;

typedef struct {
	t_list* items;
	t_entrenador* entrenador;
} t_datos_mapa;

typedef struct {
	char *nombre;
	char *tipo;
	char caracter; //Caracter que va a representar al Pokenest en el Mapa
	t_posicion* posicion;
	int cantPokemons; //registrar recursos
	t_queue *pokemons;
} t_pokenest;

typedef struct {
	char *nombre;
	int nivel;
	char carater; //Caracter que representa al Pokemon
	char *nombre_archivo; //Nombre del Archivo Correspondiente al Pokemon
	char pokenest_id; //Caracter que va a representar al Pokenest
} t_pokemon;

//Estructuras para almacenar mensajes de los Entrenadores
typedef struct {
	int fd;
	t_queue *mensajes;
} t_mensajes;

//Estructura utilizada para Administracion de Hilos de Entrenadores Bloqueados
typedef struct {
	int sem_index; //Index del semaforo correspondiente a la IO
	t_queue *entrenadores;
} t_entrenadores_bloqueados;

typedef struct {
	t_entrenador *entrenador;
	int marcado;
	int *solicitud;
	int *asignacion;
} t_entrenador_interbloqueado;

#endif /* ESTRUCTURAS_H_ */
