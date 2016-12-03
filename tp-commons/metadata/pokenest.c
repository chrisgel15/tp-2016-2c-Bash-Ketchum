/*
 * pokenest.c
 *
 *  Created on: 15/9/2016
 *      Author: utnso
 */

#include "pokenest.h"

t_config * get_pokenest_metadata(char * ruta_pokedex , char * nombre_mapa, char * nombre_pokenest)
{
	char * path = string_new();
	string_append(&path, ruta_pokedex);
	string_append(&path, "/");
	string_append(&path, MAPA_FOLDER);
	string_append(&path, "/");
	string_append(&path, nombre_mapa);
	string_append(&path, "/");
	string_append(&path, POKENEST_FOLDER);
	string_append(&path, "/");
	string_append(&path, nombre_pokenest);
	string_append(&path, "/");
	string_append(&path, NOMBRE_ARCHIVO_METADATA);

	return creacion_config_with_path(path);
}

char * get_pokenest_tipo(t_config * metadata)
{
	return config_get_string_value(metadata, TIPO_POKENEST);
}

t_posicion_pokemon * get_posicion_pokemon(t_config * metadata)
{
	char * str_posicion = config_get_string_value(metadata, POSICION_PKM);
	char ** array_posicion = string_split(str_posicion , SEPARADOR);
	t_posicion_pokemon * posicion_pokemon = malloc(sizeof(t_posicion_pokemon));

	posicion_pokemon->posicion_x = string_new();
	string_append(&(posicion_pokemon->posicion_x), array_posicion[0]);

	posicion_pokemon->posicion_y = string_new();
	string_append(&(posicion_pokemon->posicion_y), array_posicion[1]);

	return posicion_pokemon;

}

char * get_identificador_pokemon(t_config * metadata)
{
	return config_get_string_value(metadata , IDENTIFICADOR);
}

char *get_pokenest_path_dir(char * ruta_pokedex , char * nombre_mapa) {
	char * path = string_new();
	string_append(&path, ruta_pokedex);
	string_append(&path, "/");
	string_append(&path, MAPA_FOLDER);
	string_append(&path, "/");
	string_append(&path, nombre_mapa);
	string_append(&path, "/");
	string_append(&path, POKENEST_FOLDER);
	return path;
}

char *get_pokenest_pokemon_path_dir(char *ruta_pokenest, char *nombre_pokemon) {
	char * path = string_new();
	string_append(&path, ruta_pokenest);
	string_append(&path, "/");
	string_append(&path, nombre_pokemon);
	return path;
}

t_config *get_pokemon_information(char *ruta_pokenest_pokemon, char *nombre_archivo) {
	char * path = string_new();
		string_append(&path, ruta_pokenest_pokemon);
		string_append(&path, "/");
		string_append(&path, nombre_archivo);

	return creacion_config_with_path(path);
}

int get_nivel_pokemon(t_config * metadata) {
	return config_get_int_value(metadata , NIVEL);
}

/* Ejemplo de uso */

//metadata = get_pokenest_metadata(ruta_pokedex, "Ciudad Paleta", "Pikachu");
//t_posicion_pokemon * pos = get_posicion_pokemon(metadata);

//Tipo=Electrico
//Posicion=23;18
//Identificador=P
///Mapas/Ciudad Paleta/PokeNests/Pikachu/metadata
// /Mapas/[nombre]/PokeNests/[nombre-de-PokeNest]/metadata

