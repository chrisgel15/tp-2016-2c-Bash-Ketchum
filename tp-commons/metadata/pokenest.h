/*
 * pokenest.h
 *
 *  Created on: 15/9/2016
 *      Author: utnso
 */

#ifndef METADATA_POKENEST_H_
#define METADATA_POKENEST_H_

#include <stdint.h>
#include <commons/config.h>
#include <commons/string.h>

#define MAPA_FOLDER "Mapas"
#define POKENEST_FOLDER "PokeNests"
#define NOMBRE_ARCHIVO_METADATA "metadata"
#define TIPO_POKENEST "Tipo"
#define POSICION_PKM "Posicion"
#define SEPARADOR ";"
#define IDENTIFICADOR "Identificador"


typedef struct {
	char * posicion_x;
	char * posicion_y;
} t_posicion_pokemon;

#endif /* METADATA_POKENEST_H_ */
