#include "Pokenest.h"

t_list *get_listado_pokenest(char *ruta_pokedex , char *nombre_mapa) {

	t_list *pokenest_list = list_create();
	int posicion_x;
	int posicion_y;

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
	char *pokenest_path = get_pokenest_path_dir(ruta_pokedex , nombre_mapa);
	char *pokenest_pokemon_path = string_new();

	if ((dir_pokenest = opendir (pokenest_path)) != NULL) {
	  /* print all the files and directories within directory */
	  while ((ent_pokenest = readdir (dir_pokenest)) != NULL) {

		  //Omite . y ..
		  if(!string_equals_ignore_case(ent_pokenest->d_name, punto_simple) && !string_equals_ignore_case(ent_pokenest->d_name, punto_doble) ){
			  //Armo el Pokenest
			  t_pokenest *pokenest = malloc(sizeof(t_pokenest));
			  pokenest->posicion = malloc(sizeof(t_posicion));
			  t_config *metadata = get_pokenest_metadata(ruta_pokedex , nombre_mapa, ent_pokenest->d_name);
			  pokenest->nombre = ent_pokenest->d_name;
			  strncpy(&pokenest->caracter, get_identificador_pokemon(metadata), 1);
			  t_posicion_pokemon *posicion = get_posicion_pokemon(metadata);
			  pokenest->posicion->x = atoi(posicion->posicion_x);
			  pokenest->posicion->y = atoi(posicion->posicion_y);
			  pokenest->tipo = get_pokenest_tipo(metadata);
			  pokenest->pokemons = list_create();

			  pokenest_pokemon_path = get_pokenest_pokemon_path_dir(pokenest_path, ent_pokenest->d_name);

			  //Agrego los Pokemons al Pokenest
			  if ((dir_pokenest_pokemon = opendir (pokenest_pokemon_path)) != NULL) {
				  while ((ent_pokenest_pokemon = readdir (dir_pokenest_pokemon)) != NULL) {


					  if(!string_equals_ignore_case(ent_pokenest_pokemon->d_name, punto_simple) && !string_equals_ignore_case(ent_pokenest_pokemon->d_name, punto_doble) && !string_equals_ignore_case(ent_pokenest_pokemon->d_name, metada_file_name)){
						  t_pokemon *pokemon = malloc(sizeof(t_pokemon));
						  t_config *metadata_pokemon = get_pokemon_information(pokenest_pokemon_path, ent_pokenest_pokemon->d_name);
						  pokemon->nivel = get_nivel_pokemon(metadata_pokemon);
						  pokemon->nombre = pokenest->nombre;
						  list_add(pokenest->pokemons, pokemon);
					  }
				  }
				  closedir (dir_pokenest_pokemon);

			  } else {
				  perror ("Could not open Directory");
				  return EXIT_FAILURE;
			  }

			  pokenest->cantPokemons = list_size(pokenest->pokemons);

			  //Agrego el Pokenest al listado
			  list_add(pokenest_list, pokenest);
		  }

	  }
	  closedir (dir_pokenest);

	  return pokenest_list;

	} else {
	  perror ("Could not open Directory");
	  return EXIT_FAILURE;
	}
}
