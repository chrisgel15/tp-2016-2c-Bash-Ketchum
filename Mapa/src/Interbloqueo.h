/*
 * Interbloqueo.h
 *
 *  Created on: 12/11/2016
 *      Author: utnso
 */

#ifndef INTERBLOQUEO_H_
#define INTERBLOQUEO_H_

#include <commons/collections/list.h>
#include "Estructuras.h"
#include "battle.h"

int chequear_pokemones_sin_asignar(int cant_recuros, int *asignados);
int chequear_disponible_menor_a_solicitud(int cant_recursos, int *solicitud, int *disponible);
void sumar_asignacion_a_disponibles(int cant_recursos, int *asignacion, int *disponible);
int recorrer_solicitudes(t_list* interbloqueados, int cant_recursos, int entrenadores_size, int *disponible);
void liberar_batalla(t_list *entrenadores);

#endif /* INTERBLOQUEO_H_ */
