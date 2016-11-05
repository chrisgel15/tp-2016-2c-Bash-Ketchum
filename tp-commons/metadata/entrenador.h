/*
 * entrenador.h
 *
 *  Created on: 14/9/2016
 *      Author: utnso
 */

#ifndef METADATA_ENTRENADOR_H_
#define METADATA_ENTRENADOR_H_

//#include "../communications/ltnCommons.h" //Chris: Lo comente porque estaba tirando conflicto de tipos por tener doble referencia desde el Proyecto del Entrenador a este .h
#include <commons/config.h>

#define ENTRENADOR_FOLDER "Entrenadores"
#define SIMBOLO_ENTRENADOR "simbolo"
#define NOMBRE_ENTRENADOR "nombre"
#define HOJA_DE_VIAJE "hojaDeViaje"
#define VIDAS "vidas"
#define REINTENTOS "reintentos"
#define NOMBRE_ARCHIVO_METADATA "metadata"
#define DIRECTORIO_BILL "directorio de bill"


t_config * get_entrenador_metadata(char * , char * );
char * get_entrenador_simbolo(t_config *);
char * get_entrenador_nombre(t_config *);
char ** get_entrenador_hoja_de_viaje(t_config *);
char ** get_entrenador_objetivos_por_mapa(t_config *, char *);
int get_entrenador_vidas(t_config *);
int get_entrenador_reintentos(t_config *);
char *get_entrenador_directorio_bill(char *ruta_pokedex , char *nombre_entrenador);

#endif /* METADATA_ENTRENADOR_H_ */
