/*
 * Interfaz_Grafica.h
 *
 *  Created on: 7/9/2016
 *      Author: utnso
 */

#ifndef INTERFAZ_GRAFICA_H_
#define INTERFAZ_GRAFICA_H_

#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include <nivel.h>
#include <tad_items.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <stdbool.h>
#include <unistd.h>

#define TRUE 1
#define FALSE 0
#define RECURSO_POSX 20
#define RECURSO_POSY 30
#define RECURSO_QTY 3
#define ENTRENADOR_ITEM_TYPE 1
#define POKENEST_ITEM_TYPE 2



typedef struct{
	int x;
	int y;
}t_posicion;

void dibujar_mapa(void);
void dibujar_mapa_vacio(t_list*);


void mover_entrenador(void);

void CrearPokenest(t_list* items, char id, int x , int y,int cant);

#endif /* INTERFAZ_GRAFICA_H_ */
