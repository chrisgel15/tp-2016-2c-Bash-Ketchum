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
#include "Estructuras.h"

#define TRUE 1
#define FALSE 0
#define RECURSO_POSX 20
#define RECURSO_POSY 30
#define RECURSO_QTY 3
#define ENTRENADOR_ITEM_TYPE 1
#define POKENEST_ITEM_TYPE 2

void dibujar_mapa(void);
void dibujar_mapa_vacio(t_list*);

void inicializar_mapa(t_list* items, t_list* pokenest_list, char *nombre_mapa);
void ingreso_nuevo_entrenador(t_list* items, t_entrenador* entrenador, char *nombre_mapa);
void mover_entrenador_en_mapa(t_list* items, t_entrenador* entrenador, char *nombre_mapa);
void disminuir_recursos_de_pokenest(t_list* items, char pokenest_id, char *nombre_mapa);
void aumentar_recursos_de_pokenest(t_list* items, char pokenest_id, char *nombre_mapa);
void eliminar_entrenador(t_list* items, char entrenador_id, char *nombre_mapa);

//void mover_entrenador(t_entrenador *, t_log* mapa_log,t_datos_mapa* datos);

void CrearPokenest(t_list* items, char id, int x , int y,int cant);

void cargar_pokenests_en_items(t_list* items,char* ruta_pokedex,char* nombre_mapa, char** pokenests);

#endif /* INTERFAZ_GRAFICA_H_ */
