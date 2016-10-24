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

typedef struct{
	int id;
	char *nombre;
	int fd;
	char caracter; //Caracter que va a representar al Entrendor en el Mapa
	t_posicion* posicion;
	char pokemon_bloqueado;
	int pokenx;		//posicion de la pokenest en el eje x
	int pokeny;		//posicion de la pokenest en el eje y
} t_entrenador;

typedef struct{
	t_list* items;
	t_entrenador* entrenador;
} t_datos_mapa;

typedef struct{
	char *nombre;
	char caracter; //Caracter que va a representar al Entrendor en el Mapa
	t_posicion* posicion;
	int cantPokemons; //registrar recursos
} t_pokenest;

#endif /* ESTRUCTURAS_H_ */
