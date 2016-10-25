/*
 * Pokenest.h
 *
 *  Created on: 24/10/2016
 *      Author: utnso
 */

#ifndef POKENEST_H_
#define POKENEST_H_

#include "Estructuras.h"
#include <commons/collections/list.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <commons/string.h>
#include <metadata/pokenest.h>
#include <commons/config.h>

t_list *get_listado_pokenest(char *ruta_pokedex , char *nombre_mapa);


#endif /* POKENEST_H_ */
