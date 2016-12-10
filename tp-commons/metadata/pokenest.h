/*
 * pokenest.h
 *
 *  Created on: 15/9/2016
 *      Author: utnso
 */

#ifndef METADATA_POKENEST_H_
#define METADATA_POKENEST_H_

#include <stdint.h>
#include <stdio.h>
#include <commons/config.h>
#include <commons/string.h>

#define MAPA_FOLDER "Mapas"
#define POKENEST_FOLDER "PokeNests"
#define NOMBRE_ARCHIVO_METADATA "metadata"
#define TIPO_POKENEST "Tipo"
#define POSICION_PKM "Posicion"
#define SEPARADOR ";"
#define IDENTIFICADOR "Identificador"
#define NIVEL "Nivel"


typedef struct {
	char * posicion_x;
	char * posicion_y;
} t_posicion_pokemon;

char *get_pokenest_path_dir(char * ruta_pokedex , char * nombre_mapa);
t_config *get_pokemon_information(char *ruta_pokenest_pokemon, char *nombre_archivo);

#endif /* METADATA_POKENEST_H_ */
