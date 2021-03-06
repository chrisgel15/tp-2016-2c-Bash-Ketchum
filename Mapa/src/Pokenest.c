#include "Pokenest.h"

void destruir_pokenest(void *pokenest);
void destruir_pokemon(void * pokemon);

t_list *get_listado_pokenest(char *ruta_pokedex , char *nombre_mapa, char *pokenest_path){

	t_list *pokenest_list = list_create();
	int posicion_x = 0;
	int posicion_y = 0;

	char *punto_simple = string_new();
	char *punto_doble = string_new();
	char *metada_file_name = string_new();
	string_append(&punto_simple, ".");
	string_append(&punto_doble, "..");
	string_append(&metada_file_name, "metadata");

	DIR *dir_pokenest;
	DIR *dir_pokenest_pokemon;
	struct dirent *ent_pokenest;
	struct dirent *ent_pokenest_pokemon;
	//char *pokenest_path = get_pokenest_path_dir(ruta_pokedex , nombre_mapa);
	char *pokenest_pokemon_path = NULL;

	t_config *metadata_pokenest = NULL;

	if ((dir_pokenest = opendir (pokenest_path)) != NULL) {
	  /* print all the files and directories within directory */
	  while ((ent_pokenest = readdir (dir_pokenest)) != NULL) {

		 //Omite . y ..
		 if(!string_equals_ignore_case(ent_pokenest->d_name, punto_simple) && !string_equals_ignore_case(ent_pokenest->d_name, punto_doble) ){

			t_pokenest *pokenest = malloc(sizeof(t_pokenest));
			pokenest->posicion = malloc(sizeof(t_posicion));
			metadata_pokenest = get_pokenest_metadata(ruta_pokedex , nombre_mapa, ent_pokenest->d_name);
			//pokenest->nombre = ent_pokenest->d_name;
			pokenest->nombre = string_duplicate(ent_pokenest->d_name);
			strncpy(&pokenest->caracter, get_identificador_pokemon(metadata_pokenest), 1);
			t_posicion_pokemon *posicion = get_posicion_pokemon(metadata_pokenest);
			pokenest->posicion->x = atoi(posicion->posicion_x);
			pokenest->posicion->y = atoi(posicion->posicion_y);
			free(posicion->posicion_x);
			free(posicion->posicion_y);
			free(posicion);
			char *tipo = (char *)get_pokenest_tipo(metadata_pokenest);
			pokenest->tipo = string_duplicate(tipo);
			free(tipo);
			//pokenest->tipo = get_pokenest_tipo(metadata_pokenest);
			pokenest->pokemons = queue_create();

			pokenest_pokemon_path = get_pokenest_pokemon_path_dir(pokenest_path, ent_pokenest->d_name);

			//Agrego los Pokemons al Pokenest
			if ((dir_pokenest_pokemon = opendir (pokenest_pokemon_path)) != NULL) {
			  while ((ent_pokenest_pokemon = readdir (dir_pokenest_pokemon)) != NULL) {

				  if(!string_equals_ignore_case(ent_pokenest_pokemon->d_name, punto_simple) && !string_equals_ignore_case(ent_pokenest_pokemon->d_name, punto_doble) && !string_equals_ignore_case(ent_pokenest_pokemon->d_name, metada_file_name)){
					  t_pokemon_mapa *pokemon = malloc(sizeof(t_pokemon_mapa));
					  t_config *metadata_pokemon = get_pokemon_information(pokenest_pokemon_path, ent_pokenest_pokemon->d_name);
					  pokemon->nivel = get_nivel_pokemon(metadata_pokemon);
					  pokemon->nombre = pokenest->nombre;
					  pokemon->nombre_archivo = string_duplicate(ent_pokenest_pokemon->d_name);
					  pokemon->pokenest_id = pokenest->caracter;
					  queue_push(pokenest->pokemons, pokemon);
					  config_destroy(metadata_pokemon);
				  }
			  }
			  closedir (dir_pokenest_pokemon);
			} else {
			  perror ("Could not open Directory");
			  exit(0);
			}

			pokenest->cantPokemons = queue_size(pokenest->pokemons);

			//Agrego el Pokenest al listado
			list_add(pokenest_list, pokenest);
			free(pokenest_pokemon_path);
			//config_destroy(metadata_pokenest);
		  }
	  }

	  closedir (dir_pokenest);
	  free(punto_simple);
	  free(punto_doble);
	  free(metada_file_name);
	  //free(pokenest_path);
	  //config_destroy(metadata_pokenest);
	  return pokenest_list;

	} else {
	  perror ("Could not open Directory");
	  exit(0);
	}
}

t_pokenest *get_pokenest_by_identificador(t_list *lista_pokenest, char identificador){
	int tamanio_lista = list_size(lista_pokenest);
	int i = 0;
	for(i = 0; i < tamanio_lista; i++){
		t_pokenest *pokenest = (t_pokenest *) list_get(lista_pokenest, i);
		if(pokenest->caracter == identificador){
			return pokenest;
		}
	}

	return NULL;
}

t_pokemon_mapa *get_pokemon_by_identificador(t_list *lista_pokenest, char identificador){
	t_pokenest *pokenest = get_pokenest_by_identificador(lista_pokenest, identificador);

	if(pokenest != NULL){
		int tamanio_cola_pokemons = queue_size(pokenest->pokemons);

		if(tamanio_cola_pokemons > 0){
			pokenest->cantPokemons = pokenest->cantPokemons - 1;
			return (t_pokemon_mapa *)queue_pop(pokenest->pokemons);
		}
	}

	return NULL;
}

void add_pokemon_pokenest(t_list *lista_pokenest, t_pokemon_mapa *pokemon){
	t_pokenest *pokenest = get_pokenest_by_identificador(lista_pokenest, pokemon->pokenest_id);

	if(pokenest != NULL){
		pokenest->cantPokemons = pokenest->cantPokemons + 1;
		queue_push(pokenest->pokemons, pokemon);
	}
}

t_pokemon_mapa *get_pokemon_mas_fuerte(t_list *pokemons){

	int cant_pokemons = list_size(pokemons);

	if(cant_pokemons > 0){
		t_list *pokemons_ordenados = list_create();
		list_add_all(pokemons_ordenados, pokemons); //Creamos una nueva lista que nos va a servir para ordenar
		bool (*ordenar_pokemons)(t_pokemon_mapa *, t_pokemon_mapa *) = comparar_nivel_pokemons;
		list_sort(pokemons_ordenados, ordenar_pokemons);
		t_pokemon_mapa *pokemon =  (t_pokemon_mapa *)list_get(pokemons_ordenados, 0);
		list_destroy(pokemons_ordenados);
		return pokemon;
	}

	return NULL;
}

bool comparar_nivel_pokemons(t_pokemon_mapa *pokemon1, t_pokemon_mapa *pokemon2){
	return pokemon1->nivel > pokemon2->nivel;
}

int get_pokenest_index_by_pokemon_id(t_list *pokenests, char id){
	int pokenests_size = list_size(pokenests);
	int i = 0;

	for(i = 0; i < pokenests_size; i++){
		t_pokenest *pokenest = (t_pokenest *) list_get(pokenests, i);
		if(pokenest->caracter == id){
			return i;
		}
	}

	return NULL;
}

void liberar_pokenest(t_list *pokenst){
	void (*element_destroyer) (void *) = destruir_pokenest;
	list_clean_and_destroy_elements(pokenst, element_destroyer);
	list_destroy(pokenst);
}

void destruir_pokenest(void *pokenest){
	t_pokenest *pokenest_destroy = (t_pokenest *)pokenest;
	free(pokenest_destroy->nombre);
	free(pokenest_destroy->posicion);
	void (*element_destroyer) (void *) = destruir_pokemon;
	queue_destroy_and_destroy_elements(pokenest_destroy->pokemons, element_destroyer);
	free(pokenest_destroy);
}

void destruir_pokemon(void * pokemon){
	t_pokemon_mapa *pokemon_destroy = (t_pokemon_mapa *)pokemon;
	free(pokemon_destroy->nombre_archivo);
	free(pokemon_destroy);
}
