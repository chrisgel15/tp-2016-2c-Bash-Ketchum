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
t_pokenest *get_pokenest_by_identificador(t_list *lista_pokenest, char identificador);
t_pokemon *get_pokemon_by_identificador(t_list *lista_pokenest, char identificador);
void add_pokemon_pokenest(t_list *lista_pokenest, t_pokemon *pokemon);
t_pokemon *get_pokemon_mas_fuerte(t_list *pokemons);
bool comparar_nivel_pokemons(t_pokemon *pokemon1, t_pokemon *pokemon2);

#endif /* POKENEST_H_ */
