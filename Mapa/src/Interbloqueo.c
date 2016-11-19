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

t_list *ordenar_entrenadores_interbloqueados(t_list *entrenadores){
	bool (*ordenar_entrenadores)(t_entrenador *, t_entrenador *) = comparar_tiempo_entrenada_entrenadores;
	list_sort(entrenadores, ordenar_entrenadores);
	return entrenadores;
}

bool comparar_tiempo_entrenada_entrenadores(t_entrenador *entrenador1, t_entrenador *entrenador2){
	return entrenador1->tiempo_ingreso < entrenador2->tiempo_ingreso;
}

t_entrenador *liberar_batalla(t_entrenador *entrenador1, t_entrenador *entrenador2){

	t_pkmn_factory* pokemon_factory = create_pkmn_factory();

	//Obtengo el primer Entreador que va a enfrentarse
	t_entrenador *victima = entrenador1;
	t_pokemon_mapa *victima_pokemon_mapa = (t_pokemon_mapa *)list_remove(victima->pokemons, 0);
	t_pokemon *victima_pokemon = create_pokemon(pokemon_factory, victima_pokemon_mapa->nombre, victima_pokemon_mapa->nivel);

	//Obtengo al adversario
	t_entrenador *adversario = entrenador2;
	t_pokemon_mapa *adversario_pokemon_mapa = (t_pokemon_mapa *)list_remove(adversario->pokemons, 0);
	t_pokemon *adversario_pokemon = create_pokemon(pokemon_factory, adversario_pokemon_mapa->nombre, adversario_pokemon_mapa->nivel);

	 //Enfrentamiento
	 t_pokemon *loser = pkmn_battle(victima_pokemon, adversario_pokemon);

	 if(adversario_pokemon == loser){
		 victima = adversario;
	 }

	 free(victima_pokemon);
	 free(adversario_pokemon);
	 destroy_pkmn_factory(pokemon_factory);

	 return victima;
}
