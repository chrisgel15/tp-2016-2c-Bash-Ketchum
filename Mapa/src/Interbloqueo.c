#include "Interbloqueo.h"

int chequear_pokemones_sin_asignar(int cant_recuros, int *asignados){
	int i;

	for(i = 0; i < cant_recuros; i++){
		if(asignados[i] != 0){
			return 0;
		}
	}

	return 1;
}

int chequear_disponible_menor_a_solicitud(int cant_recursos, int *solicitud, int *disponible)
{
	int i;

	for(i = 0; i < cant_recursos; i++){
		if(solicitud[i] > disponible[i]){
			return 0;
		}
	}

	return 1;
}

void sumar_asignacion_a_disponibles(int cant_recursos, int *asignacion, int *disponible){
	int i;

	for(i = 0; i < cant_recursos; i++){
		disponible[i] = disponible[i] + asignacion[i];
	}
}

int recorrer_solicitudes(t_list* interbloqueados, int cant_recursos, int entrenadores_size, int *disponible){
	int i;

	for(i = 0; i < entrenadores_size; i++){
		t_entrenador_interbloqueado *entrenador = list_get(interbloqueados, i);
		//Valido si el Entrenador no esta marcado
		if(!entrenador->marcado){
			if(chequear_disponible_menor_a_solicitud(cant_recursos, entrenador->solicitud, disponible)){
				entrenador->marcado = 1; //Marco al Entrenador
				sumar_asignacion_a_disponibles(cant_recursos, entrenador->asignacion, disponible); //Sumo el vector asignacion a Disponible
				return 1;
			}
		}
	}

	return 0;
}

t_entrenador *liberar_batalla(t_list *entrenadores){

}
