/*
 * Interfaz_Grafica.c

 *
 *  Created on: 7/9/2016
 *      Author: utnso
 */

#include "Mapa.h"

int distancia_A_RecursoH(int);
int distancia_A_RecursoY(int);

int distancia_A_RecursoH(int posx){
	int dist = RECURSO_POSX - posx;
	return dist;
}
int distancia_A_RecursoY(int posy){

	int dist = RECURSO_POSY - posy;
	return dist;
}


void dibujar_mapa() {

	int filas, columnas;
	int distanciaH, distanciaY;

	t_list* items = list_create();

	t_posicion* pos = malloc(sizeof(t_posicion));
	t_entrenador* personaje1 = malloc(sizeof(t_entrenador));

	/*iNICIALIZO EL PERSONAJE - ID Y POSICION*/
	personaje1->caracter = '@';
	personaje1->posicion = pos;
	personaje1->posicion->x = 0;
	personaje1->posicion->y = 0;
	/*--------------------------------------*/

	CrearPersonaje(items, personaje1->caracter, personaje1->posicion->x,
			personaje1->posicion->y);
	CrearCaja(items, 'C', RECURSO_POSX, RECURSO_POSY, RECURSO_QTY);

	nivel_gui_inicializar();
	nivel_gui_get_area_nivel(&filas, &columnas);

	nivel_gui_dibujar(items, "Test Chamber 04");

	distanciaH = TRUE;
	distanciaY = TRUE;
	while (distanciaH || distanciaY) {

		distanciaH = distancia_A_RecursoH(personaje1->posicion->x);
		distanciaY = distancia_A_RecursoY(personaje1->posicion->y);

		if (distanciaH > 0) {
			personaje1->posicion->x++;
			MoverPersonaje(items, personaje1->caracter, personaje1->posicion->x,
					personaje1->posicion->y);
			sleep(1);
			nivel_gui_dibujar(items, "Test Chamber 04");
		}

		if (distanciaY > 0) {
			personaje1->posicion->y++;
			MoverPersonaje(items, personaje1->caracter, personaje1->posicion->x,
					personaje1->posicion->y);
			sleep(1);
			nivel_gui_dibujar(items, "Test Chamber 04");
		}
	}

	restarRecurso(items, 'C');
	sleep(1);
	nivel_gui_dibujar(items, "Test Chamber 04");
	sleep(3);

	nivel_gui_terminar();
	free(pos);
	free(personaje1);

}

void dibujar_mapa_vacio(){
	t_list* items = list_create();
	int filas, columnas;
	nivel_gui_inicializar();
	nivel_gui_get_area_nivel(&filas, &columnas);
	nivel_gui_dibujar(items, "Mapa Checkpoint I");
}

