/*
 * Gps.c
 *
 *  Created on: 18/9/2016
 *  Author: Leandro
 *
 */

#include "Gps.h"

/************** METODOS PARA INICIALIZAR ESTRUCTURAS **************/
t_posicion_mapa *inicializar_posicion_entrenador(){
	t_posicion_mapa *posicion = malloc(sizeof(t_posicion_mapa));
	return posicion;
}

t_posicion_pokenest *inicializar_posicion_pokenest(){
	t_posicion_pokenest *pokenest = malloc(sizeof(t_posicion_pokenest));
	return pokenest;
}

/************** METODOS PARA ACTUALIZAR ESTRUCUTRAS **************/
void ingresar_a_nuevo_mapa(t_posicion_mapa *posicion){
	posicion->x = 0;
	posicion->y = 0;
}

void actualizar_posicion_pokenest(t_posicion_pokenest *pokenest, int posicion_x, int posicion_y){
	pokenest->x = posicion_x;
	pokenest->y = posicion_y;
}

/************** METODOS PARA MOVERSE POR EL MAPA **************/
int moverse_en_mapa_eje_x(t_posicion_mapa *posicion,  t_posicion_pokenest *pokenest){
	int moverse = 0;

	if(posicion->x < pokenest->x){
		posicion->x = posicion->x + 1;
		moverse = 1;
	}

	if(posicion->x > pokenest->x){
		posicion->x = posicion->x -1 ;
		moverse = -1;
	}

	return moverse;
}

int moverse_en_mapa_eje_y(t_posicion_mapa *posicion,  t_posicion_pokenest *pokenest){
	int moverse = 0;

	if(posicion->y < pokenest->y){
		posicion->y = posicion->y + 1;
		moverse = 1;
	}

	if(posicion->y > pokenest->y){
		posicion->y = posicion->y -1 ;
		moverse = -1;
	}

	return moverse;
}
