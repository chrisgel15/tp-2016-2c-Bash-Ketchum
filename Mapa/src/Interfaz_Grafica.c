/*
 * Interfaz_Grafica.c

 *
 *  Created on: 7/9/2016
 *      Author: utnso
 */

#include "Mapa.h"

/*int distancia_A_RecursoH(int);
int distancia_A_RecursoY(int);
void mover_eje_x(t_datos_mapa*,int,char*);
void mover_eje_y(t_datos_mapa*,int,char*);

int distancia_A_RecursoH(int posx){
	int dist = RECURSO_POSX - posx;
	return dist;
}
int distancia_A_RecursoY(int posy){

	int dist = RECURSO_POSY - posy;
	return dist;
}*/



//extern pthread_mutex_t mutex_desplaza_x;
//extern pthread_mutex_t mutex_desplaza_y;

//Semaforo utilizado para la interfaz
pthread_mutex_t mutex_interfaz;

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

/*void dibujar_mapa_vacio(t_list* items){
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
}*/

void inicializar_mapa(t_list* items, t_list* pokenest_list, char *nombre_mapa){
	int filas = 50;
	int columnas = 50;
	int i = 0;
	int cantidad_pokenest = list_size(pokenest_list);
	pthread_mutex_init(&mutex_interfaz, NULL); //Inicializo Mutex Para La Interfaz

	for(i = 0; i < cantidad_pokenest; i++){
		t_pokenest *pokenest = (t_pokenest *) list_get(pokenest_list, i);
		CrearCaja(items, pokenest->caracter, pokenest->posicion->x, pokenest->posicion->y, pokenest->cantPokemons);
	}
	nivel_gui_inicializar();
	nivel_gui_get_area_nivel(&filas, &columnas);
	nivel_gui_dibujar(items, nombre_mapa);
}

void ingreso_nuevo_entrenador(t_list* items, t_entrenador* entrenador, char *nombre_mapa){
	pthread_mutex_lock(&mutex_interfaz);
	CrearPersonaje(items, entrenador->caracter, entrenador->posicion->x, entrenador->posicion->y);
	nivel_gui_dibujar(items, nombre_mapa);
	sleep(1);
	pthread_mutex_unlock(&mutex_interfaz);
}

void mover_entrenador_en_mapa(t_list* items, t_entrenador* entrenador, char *nombre_mapa){
	pthread_mutex_lock(&mutex_interfaz);
	MoverPersonaje(items, entrenador->caracter, entrenador->posicion->x, entrenador->posicion->y);
	nivel_gui_dibujar(items, nombre_mapa);
	sleep(1);
	pthread_mutex_unlock(&mutex_interfaz);
}

void disminuir_recursos_de_pokenest(t_list* items, char pokenest_id, char *nombre_mapa){
	pthread_mutex_lock(&mutex_interfaz);
	restarRecurso(items, pokenest_id);
	nivel_gui_dibujar(items, nombre_mapa);
	sleep(1);
	pthread_mutex_unlock(&mutex_interfaz);
}

void aumentar_recursos_de_pokenest(t_list* items, char pokenest_id, char *nombre_mapa){
	pthread_mutex_lock(&mutex_interfaz);

	ITEM_NIVEL* item = _search_item_by_id(items, pokenest_id);

	if (item != NULL) {
		item->quantity = item->quantity + 1;
	} else {
		printf("WARN: Item %c no existente\n", pokenest_id);
	}

	nivel_gui_dibujar(items, nombre_mapa);
	sleep(1);
	pthread_mutex_unlock(&mutex_interfaz);
}

void eliminar_entrenador(t_list* items, char entrenador_id, char *nombre_mapa){
	pthread_mutex_lock(&mutex_interfaz);
	BorrarItem(items, entrenador_id);
	nivel_gui_dibujar(items, nombre_mapa);
	sleep(1);
	pthread_mutex_unlock(&mutex_interfaz);
}

//TODO: @Gi - No hace falta usar la funcion CrearItem -> La ibreria ya ofrece funciones
//		CrearCaja y CrearPersonaje que implementan CrearItem.
//		Debería ser así: CrearPkenest(itms,id,x,y,cantidad){
//							CrearCaja(items,id,x,y,cantidad);
//						 }
/*void CrearPokenest(t_list* items, char id, int x , int y,int cant){
	 CrearItem(items, id, x, y, POKENEST_ITEM_TYPE, cant);
}

//TODO: @Gi - Idem Pokenest
void CrearEntrenador (t_list* items, char id, int x , int y){
	CrearItem(items, id, x, y, ENTRENADOR_ITEM_TYPE,0);
}

void posicionar_entrenador_en_mapa(t_datos_mapa* datos, char * nombre_mapa){
	CrearEntrenador(datos->items,datos->entrenador->caracter,
			datos->entrenador->posicion->x,datos->entrenador->posicion->y);
	nivel_gui_dibujar(datos->items,nombre_mapa);
}

void posicionar_pokenest_en_mapa(t_datos_mapa* datos, char * nombre_mapa){
	nivel_gui_dibujar(datos->items,nombre_mapa);
}


void mover_entrenador_hacia_recurso(t_datos_mapa* datos,char* nombre_mapa){
	int distanciaH = TRUE;
	int distanciaY = TRUE;

	while(distanciaH || distanciaY){

		pthread_mutex_lock(&mutex_desplaza_x);
			distanciaH = distancia_A_RecursoH(datos->entrenador->posicion->x);
		pthread_mutex_unlock(&mutex_desplaza_x);

		mover_eje_x(datos,distanciaH,nombre_mapa);

		pthread_mutex_lock(&mutex_desplaza_y);
			distanciaY = distancia_A_RecursoY(datos->entrenador->posicion->y);
		pthread_mutex_unlock(&mutex_desplaza_y);

		mover_eje_y(datos,distanciaY,nombre_mapa);
	}
}

void mover_eje_x(t_datos_mapa* datos, int dist,char * nombre_mapa) {
	if (dist > 0) {
		datos->entrenador->posicion->x++;
		MoverPersonaje(datos->items, datos->entrenador->caracter,
				datos->entrenador->posicion->x, datos->entrenador->posicion->y);
		sleep(1);
		nivel_gui_dibujar(datos->items, nombre_mapa);
	}
}

void mover_eje_y(t_datos_mapa* datos, int dist,char* nombre_mapa) {
	if (dist > 0) {
		datos->entrenador->posicion->y++;
		MoverPersonaje(datos->items, datos->entrenador->caracter,
				datos->entrenador->posicion->x, datos->entrenador->posicion->y);
		sleep(1);
		nivel_gui_dibujar(datos->items, nombre_mapa);
	}
}


void mover_entrenador(t_entrenador *entrenador, t_log* mapa_log,t_datos_mapa* datos,char* nombre_mapa){

	//t_entrenador* entrenador=malloc(sizeof(t_entrenador));
	int *result = malloc(sizeof(int));
	int posx, posy;
	//entrenador=buscar_entrenador(fd_entrenador);
	datos->entrenador=entrenador;
	posx = recibirInt(entrenador->fd, result, mapa_log);
	posy = recibirInt(entrenador->fd, result, mapa_log);
	mover_eje_x(datos,posx,nombre_mapa);
	mover_eje_y(datos, posy,nombre_mapa);

}*/


/***** FUNCION:  OBTENER DATOS DE POKENEST Y AGREGARLO A LA LISTA ITEMS PARA DIBUJAR EN EL MAPA *****/
/*void cargar_pokenests_en_items(t_list* items,char* ruta_pokedex,char* nombre_mapa, char** pokenests){
	int i=0;
	t_config* metadata_pokenest;
	t_posicion* posicion;
	ITEM_NIVEL* pokenest;

	while (pokenests[i]!=NULL){
		metadata_pokenest = get_pokenest_metadata( ruta_pokedex , nombre_mapa, pokenests[i]);
		pokenest = malloc(sizeof(ITEM_NIVEL));
		posicion = get_posicion_pokemon(metadata_pokenest);
		pokenest->posx = posicion->x;
		pokenest->posy = posicion->y;
		pokenest->item_type=POKENEST_ITEM_TYPE;
		pokenest->id = get_identificador_pokemon(metadata_pokenest);
		pokenest->quantity=2;//TODO OBTENER LA CANTIDAD DE LA CANTIDAD DE ARCHIVOS .DATA
		CrearPokenest(items, pokenest->id, pokenest->posx , pokenest->posy, pokenest->quantity);


	}

}*/
