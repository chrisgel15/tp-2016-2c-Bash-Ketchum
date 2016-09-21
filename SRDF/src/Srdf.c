/*
 ============================================================================
 Name        : Srdf.c
 Author      : Federico Mandri
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */


#include "Srdf.h"

void crearEntrenador(t_list*,char,int,int);
void crearPokeNest(t_list*,char,int,int,int);
void inicializar_colas(void);
void inicializar_listas(void);
t_entrenador* inicializar_info_entrenador(char*, char, int, int);
void srdf(t_list*);
bool menor_distancia(t_entrenador*, t_entrenador*);
double distancia_a_pokenest(t_entrenador*);


t_list* cola_listos;
t_queue* cola_ejecucion;
t_list* entrenadores;
t_list* items;

int main(void) {
	//int filas, columnas;
	inicializar_colas();
	inicializar_listas();

	//crearPokeNest(items,'P',10,10,5);
//	list_add(cola_listos,(void*)inicializar_info_entrenador("Ash", '@', 1, 1));
//	list_add(cola_listos,(void*)inicializar_info_entrenador("Gary", '#', 20, 20));
	//list_add_all(items,cola_listos);
//	crearEntrenador(items,'@',1,1);
//	CrearEnemigo(items,'#',20,20);
//	nivel_gui_inicializar();
//	nivel_gui_get_area_nivel(&filas, &columnas);
//	nivel_gui_dibujar(items, "Test SRDF");
//	sleep(5);
//	nivel_gui_terminar();
	//srdf(cola_listos);


	return EXIT_SUCCESS;
}


void crearEntrenador(t_list* items, char id, int x, int y){
	CrearPersonaje(items,id,x,y);
}

void crearPokeNest(t_list* items, char id, int x, int y, int cantidad){
	CrearCaja(items,id,x,y,cantidad);
}

void inicializar_colas(void){
	cola_ejecucion = queue_create();

}

void inicializar_listas(void){
	cola_listos = list_create();
	entrenadores = list_create();
	items = list_create();
}

t_entrenador* inicializar_info_entrenador(char* nombre, char id, int x, int y){
	t_entrenador* nuevo_entrenador = malloc(sizeof(t_entrenador));
	nuevo_entrenador->nombre = nombre;
	nuevo_entrenador->id = id;
	nuevo_entrenador->posx = x;
	nuevo_entrenador->posy = y;

	return nuevo_entrenador;
}

void srdf(t_list* entrenadores_listos){

	bool (*pf)(t_entrenador*,t_entrenador*) = menor_distancia;
	list_sort(entrenadores_listos, pf);
}


bool menor_distancia(t_entrenador* unEntrenador, t_entrenador* otroEntrenador){
	int unaDistancia = distancia_a_pokenest(unEntrenador);
	int otraDistancia = distancia_a_pokenest(otroEntrenador);
	return (unaDistancia < otraDistancia);
}

double distancia_a_pokenest(t_entrenador* entrenador){
	int deltaX = entrenador->pokenx - entrenador->posx;
	int deltaY = entrenador->pokeny - entrenador->posy;

	return sqrt(deltaX*deltaX + deltaY*deltaY);

}

