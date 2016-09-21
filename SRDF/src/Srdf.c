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
bool menor_distancia(t_entrenador*, t_entrenador*);
double distancia_a_pokenest(t_entrenador*);
void set_datos(void);


t_list* cola_listos;
t_queue* cola_ejecucion;
t_list* entrenadores;
t_list* items;

int main(void) {
	//int filas, columnas;
	t_entrenador* aux = malloc(sizeof(t_entrenador));
	inicializar_colas();
	inicializar_listas();
	set_datos();
	int elementos_cola = list_size(cola_listos);
	int i = 0;
	int j = 0;
	for(i; i < elementos_cola; i++){
		aux = list_get(cola_listos,i);
		printf("Entrenadores antes de ordenar %c\n", aux->id);
	}
	srdf(cola_listos);
	for (j; j < elementos_cola; j++) {
		aux = list_get(cola_listos, j);
		printf("Entrenadores despues de ordenar %c\n", aux->id);
	}




	return EXIT_SUCCESS;
}

void set_datos(void){
	t_entrenador* entrenador1 = inicializar_info_entrenador("Ash",'@',0,0);
	t_entrenador* entrenador2 = inicializar_info_entrenador("Gary",'#',19,18);
	t_entrenador* entrenador3 = inicializar_info_entrenador("Brook",'$',15,17);
	entrenador1 -> pokenx = 20;
	entrenador1 -> pokeny = 20;
	entrenador2 -> pokenx = 20;
	entrenador2 -> pokeny = 20;
	entrenador3 -> pokenx = 20;
	entrenador3 -> pokeny = 20;
	list_add(cola_listos,(void*)entrenador1);
	list_add(cola_listos,(void*)entrenador2);
	list_add(cola_listos,(void*)entrenador3);
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

