/*
 * PokedexServidor.h
 *
 *  Created on: 19/11/2016
 *      Author: utnso
 */

#ifndef POKEDEXSERVIDOR_H_
#define POKEDEXSERVIDOR_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
//#include <signal.h>
#include <fuse.h>
#include <commons/bitarray.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/temporal.h>
#include <communications/ltnCommons.h>
#include <pthread.h>
//#include <semaphore.h>
#include "Osada.h"

void UbicarPunteros();
int CantidadBloquesLibres(int condicion);
char * TipoDeArchivo (int tipo);
int TamanioEnBloques(int tamanioBytes);

void RecibirYProcesarPedido(int fdCliente);
void ProcesarGetAttr(int fd);
void ProcesarReadDir(int fd);
void SetAttrByDirectoryId(int directoryId, int fd);
bool FindDirectoryByName(char * path, int * directoryId);
bool FindDirectoryByNameAndParent(char ** path, int  parentId, int * directoryId);
void FindAllFilesByParentId(int * parentId, void *buf, fuse_fill_dir_t filler, int fd);
char ** SplitPath(char * path);

// Test
void ImprimirHeader();
void ImprimirBitMap(t_bitarray *);
void ImprimirTablaDeArchivos();
void ImprimirBloquesDelBitMap(t_bitarray * bitmap, int size, int * parcial, int * total);
void ImprimirUnBloqueDeBits(t_bitarray * bitmap, int * total, int * parcial);
void ImprimirBloquesDeTablaAsignacion(int * tabla_asignaciones, int file_size, int first_block);
void GenerarArchivo(char * indice_datos, int tamanio_en_bytes, int primer_bloque, char * nombre_archivo);



// Defines
#define OSADA_BLOCK_SIZE 64
#define OSADA_TABLA_ARCHIVOS_SIZE 1024
#define OSADA_CANTIDAD_MAXIMA_ARCHIVOS 2048
#define OSADA_HEADER_BLOCK_SIZE 1
#define NO_HAY_ESPACIO_TABLA_ARCHIVOS -1
#define NO_HAY_ESPACIO_BITMAP -1
#define TAMANIO_MAXIMO_NOMBRE_ARCHIVO 17
#define CONTAR_TODOS_LOS_BLOQUES -1
#define SIN_BLOQUES_ASIGNADOS 0xFFFFFFFF
#define LECTURA 1
#define ESCRITURA 2
#define NO_HAY_MAS_BLOQUES_POR_RW 0
#define POKEDEX_SERVIDOR_PUERTO_ESCUCHA "POKEDEX_SERVIDOR_PUERTO_ESCUCHA"


// Variables Globales
t_log * osada_log;
t_config* osada_server_config;
int fd_osadaDisk;
struct stat osadaStat;
int* pmap_osada;
int tamanio_tabla_asignaciones_bloques;

// Punteros a las diferentes estructuras del disco.
osada_header* header;
t_bitarray * bitmap;
osada_file * tabla_archivos;
int * tabla_asignaciones;
char * tabla_datos;
int offset_bitmap_datos; // offset dentro del bitmap que apunta al comienzo de la tabla de datos

#endif /* POKEDEXSERVIDOR_H_ */
