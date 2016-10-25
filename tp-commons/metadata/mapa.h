/*
 * mapa.h
 *
 *  Created on: 15/9/2016
 *      Author: utnso
 */

#ifndef METADATA_MAPA_H_
#define METADATA_MAPA_H_

#include "../communications/ltnCommons.h"

#define MAPA_FOLDER "Mapas"
#define NOMBRE_ARCHIVO_METADATA "metadata"
#define TIEMPO_DEADLOCK "TiempoChequeoDeadlock"
#define BATALLA "Batalla"
#define ALGORITMO "algoritmo"
#define QUANTUM "quantum"
#define RETARDO "retardo"
#define IP "IP"
#define PUERTO "Puerto"

t_config * get_mapa_metadata(char * ruta_pokedex , char * nombre_mapa);
int get_mapa_tiempo_deadlock(t_config * metadata);
int get_mapa_batalla_on_off(t_config * metadata);
char * get_mapa_algoritmo(t_config * metadata);
int get_mapa_quantum(t_config * metadata);
int get_mapa_retardo(t_config * metadata);
char * get_mapa_ip(t_config * metadata);
int get_mapa_puerto(t_config * metadata);
char ** get_lista_de_pokenest(t_config * metadata);
char *get_mapa_path(char * ruta_pokedex , char * nombre_mapa);


#endif /* METADATA_MAPA_H_ */
