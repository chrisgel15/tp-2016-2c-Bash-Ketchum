/*
 * Interfaz_Grafica.c

 *
 *  Created on: 7/9/2016
 *      Author: utnso
 */

#include "Mapa.h"

int distancia_A_RecursoH(int);
int distancia_A_RecursoY(int);
void mover_eje_x(t_datos_mapa*,int);
void mover_eje_y(t_datos_mapa*,int);

int distancia_A_RecursoH(int posx){
	int dist = RECURSO_POSX - posx;
	return dist;
}
int distancia_A_RecursoY(int posy){

	int dist = RECURSO_POSY - posy;
	return dist;
}



extern pthread_mutex_t mutex_desplaza_x;
extern pthread_mutex_t mutex_desplaza_y;

/*F:MANDRI => funcion dibujar_mapa(): ejemplo para ver como mover un personaje hacia un recurso
 * para probar se puede armar un proyecto separado (fuera del repo mejor)
 * incluir Interfaz_grafica.h y la estructura t_entrenador que se movio a mapa.h*
 *
 * Queda comentada por si sirve el codigo para encapsular comportamiento en nuevas funciones*/

//void dibujar_mapa() {
//
//	int filas, columnas;
//	int distanciaH, distanciaY;
//
//	t_list* items = list_create();
//
//	t_posicion* pos = malloc(sizeof(t_posicion));
//	t_entrenador* personaje1 = malloc(sizeof(t_entrenador));
//
//	/*iNICIALIZO EL PERSONAJE - ID Y POSICION*/
//	personaje1->caracter = '@';
//	personaje1->posicion = pos;
//	personaje1->posicion->x = 0;
//	personaje1->posicion->y = 0;
//	/*--------------------------------------*/
//
//	CrearPersonaje(items, personaje1->caracter, personaje1->posicion->x,
//			personaje1->posicion->y);
//	CrearCaja(items, 'C', RECURSO_POSX, RECURSO_POSY, RECURSO_QTY);
//
//	nivel_gui_inicializar();
//	nivel_gui_get_area_nivel(&filas, &columnas);
//
//	nivel_gui_dibujar(items, "Test Chamber 04");
//
//	distanciaH = TRUE;
//	distanciaY = TRUE;
//	while (distanciaH || distanciaY) {
//
//		distanciaH = distancia_A_RecursoH(personaje1->posicion->x);
//		distanciaY = distancia_A_RecursoY(personaje1->posicion->y);
//
//		if (distanciaH > 0) {
//			personaje1->posicion->x++;
//			MoverPersonaje(items, personaje1->caracter, personaje1->posicion->x,
//					personaje1->posicion->y);
//			sleep(1);
//			nivel_gui_dibujar(items, "Test Chamber 04");
//		}
//
//		if (distanciaY > 0) {
//			personaje1->posicion->y++;
//			MoverPersonaje(items, personaje1->caracter, personaje1->posicion->x,
//					personaje1->posicion->y);
//			sleep(1);
//			nivel_gui_dibujar(items, "Test Chamber 04");
//		}
//	}
//
//	restarRecurso(items, 'C');
//	sleep(1);
//	nivel_gui_dibujar(items, "Test Chamber 04");
//	sleep(3);
//
//	nivel_gui_terminar();
//	free(pos);
//	free(personaje1);
//
//}

void dibujar_mapa_vacio(t_list* items){
	//t_list* items = list_create();
	int filas = 50;
	int columnas = 50;;
	CrearPokenest(items, 'C', RECURSO_POSX, RECURSO_POSY, RECURSO_QTY);
	CrearPokenest(items, 'P', 15, 20, 5);
	nivel_gui_inicializar();
	nivel_gui_get_area_nivel(&filas, &columnas);
	nivel_gui_dibujar(items, "Mapa Checkpoint I");
	//Termino el mapa despues de 10 segundos
//	sleep(10);
//	nivel_gui_terminar();
}


void CrearPokenest(t_list* items, char id, int x , int y,int cant){
	 CrearItem(items, id, x, y, POKENEST_ITEM_TYPE, cant);
}

void CrearEntrenador (t_list* items, char id, int x , int y){
	CrearItem(items, id, x, y, ENTRENADOR_ITEM_TYPE,0);
}

void posicionar_entrenador_en_mapa(t_datos_mapa* datos){
	CrearEntrenador(datos->items,datos->entrenador->caracter,
			datos->entrenador->posicion->x,datos->entrenador->posicion->y);
	nivel_gui_dibujar(datos->items,"Mapa Checkpoint I");
}

void posicionar_pokenest_en_mapa(t_datos_mapa* datos){
	nivel_gui_dibujar(datos->items,"Mapa Checkpoint I");
}


void mover_entrenador_hacia_recurso(t_datos_mapa* datos){
	int distanciaH = TRUE;
	int distanciaY = TRUE;

	while(distanciaH || distanciaY){

		pthread_mutex_lock(&mutex_desplaza_x);
			distanciaH = distancia_A_RecursoH(datos->entrenador->posicion->x);
		pthread_mutex_unlock(&mutex_desplaza_x);

		mover_eje_x(datos,distanciaH);

		pthread_mutex_lock(&mutex_desplaza_y);
			distanciaY = distancia_A_RecursoY(datos->entrenador->posicion->y);
		pthread_mutex_unlock(&mutex_desplaza_y);

		mover_eje_y(datos,distanciaY);
	}
}

void mover_eje_x(t_datos_mapa* datos, int dist) {
	if (dist > 0) {
		datos->entrenador->posicion->x++;
		MoverPersonaje(datos->items, datos->entrenador->caracter,
				datos->entrenador->posicion->x, datos->entrenador->posicion->y);
		sleep(1);
		nivel_gui_dibujar(datos->items, "Mapa Checkpoint I");
	}
}

void mover_eje_y(t_datos_mapa* datos, int dist) {
	if (dist > 0) {
		datos->entrenador->posicion->y++;
		MoverPersonaje(datos->items, datos->entrenador->caracter,
				datos->entrenador->posicion->x, datos->entrenador->posicion->y);
		sleep(1);
		nivel_gui_dibujar(datos->items, "Mapa Checkpoint I");
	}
}


void mover_entrenador(){

}
