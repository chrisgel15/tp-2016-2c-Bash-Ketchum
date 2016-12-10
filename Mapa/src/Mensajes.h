/*
 * Mensajes.h
 *
 *  Created on: 28/10/2016
 *      Author: utnso
 */

#ifndef MENSAJES_H_
#define MENSAJES_H_

#include "Estructuras.h"
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <communications/instructionCodes.h>
#include <commons/log.h>
#include <semaphore.h>


void inicializar_semaforo_mensajes();
void inicializar_mensajes_entrenador(t_list *mensajes, int fd, t_log *log);
void agregar_nuevo_mensaje(t_list *mensajes_entrenadores, int fd, void *mensaje);
void *obtener_mensaje(t_mensajes *mensajes);
t_mensajes *obtener_mensajes_de_entrenador(t_list *mensajes_entrenadores, int fd);
int recibir_mensaje_ubicacion_pokenest(t_list *mensajes_entrenadores, int fd, t_log *log);
int recibir_mensaje_avanzar_hacia_pokenest(t_list *mensajes_entrenadores, int fd, t_log *log);
int recibir_mensaje_atrapar_pokemon(t_list *mensajes_entrenadores, int fd, t_log *log);
int recibir_mensaje_objetivo_cumplido(t_list *mensajes_entrenadores, int fd, t_log *log);
void eliminar_cola_mensajes_entrenador(t_list *mensajes, int fd, t_log *log);
void entrenador_desconectado(t_list *mensajes_entrenadores, int fd);
void liberar_mensajes(t_list *mensajes, t_log *log);

#endif /* MENSAJES_H_ */
