/*
 * Gps.h
 *
 *  Created on: 18/9/2016
 *  Author: Leandro
 *  Desde aca vamos a controlar la posicion tanto del Entrenador como del proximo Pokenest.
 *  Ademas nos permite movernos a traves del Mapa
 */

#ifndef GPS_H_
#define GPS_H_

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

typedef struct{
	int x;
	int y;
} t_posicion_pokenest;

typedef struct{
	int x;
	int y;
} t_posicion_mapa;

t_posicion_mapa *inicializar_posicion_entrenador();
t_posicion_pokenest *inicializar_posicion_pokenest();
void ingresar_a_nuevo_mapa(t_posicion_mapa *posicion);
void actualizar_posicion_pokenest(t_posicion_pokenest *pokenest, int posicion_x, int posicion_y);
int moverse_en_mapa_eje_x(t_posicion_mapa *posicion,  t_posicion_pokenest *pokenest);
int moverse_en_mapa_eje_y(t_posicion_mapa *posicion,  t_posicion_pokenest *pokenest);
int llego_a_pokenest(t_posicion_mapa *posicion,  t_posicion_pokenest *pokenest);

#endif /* GPS_H_ */
