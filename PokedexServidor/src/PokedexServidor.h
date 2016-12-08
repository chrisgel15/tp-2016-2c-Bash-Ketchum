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
#include <time.h>
//#include <signal.h>
#include <fuse.h>
#include <commons/bitarray.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/temporal.h>
#include <communications/ltnCommons.h>
#include <pthread.h>
#include <semaphore.h>
#include "Osada.h"

void UbicarPunteros();
int CantidadBloquesLibres(int condicion);
char * TipoDeArchivo (int tipo);
int TamanioEnBloques(int tamanioBytes);

void CrearHilo(int fdCliente);
void RecibirYProcesarPedido(int fdCliente);
void ProcesarGetAttr(int fd);
void ProcesarReadDir(int fd);
void ProcesarRead(int fd);
void ProcesarMkDir(int fd);
void ProcesarCreate(int fdCliente);
void ProcesarWrite(int fdCliente);
void ProcesarUnlink(int fdCliente);
void ProcesarTruncate(int fd);
void ProcesarRmDir(int fd);
void ProcesarRename(int fdCliente);
void ProcesarUtimens(int fdCliente);
void SetAttrByDirectoryId(int directoryId, int fd);
bool FindDirectoryByName(char * path, int * directoryId);
bool FindDirectoryByNameAndParent(char ** path, int  parentId, int * directoryId);
void FindAllFilesByParentId(int * parentId, void *buf, fuse_fill_dir_t filler, int fd);
char ** SplitPath(char * path);
int FinalDeBloqueLectoEscritura(int * indice_tabla_asignaciones, int bloque_actual, int operacion, int directoryId);
int AsignarBloqueActualLectoEscritura(osada_file * indice_tabla_archivos, int operacion, int directoryId);
int BuscaPrimerEspacioDisponibleEnBitMap(int directoryId);
int Crear(char * path, int state);
int BuscaPrimerEspacioDisponibleEnTablaArchivos();
bool TamanioNombreAdecuado(char * path);
bool ExistsDirectoryByName(char * path);
bool FindParentDirectoryByName(char * path, int * directoryId);
int LongitudArray(char ** arr);
void CrearArchivoDirectorio(char * path, int tablaArchivosId, int parentDirectoryId, int state);
int DeleteBlocks(size_t blocks_to_delete, int directoryId);
void SeteaBitEnBitMap(int offset);
void RollbackLectoEscrituraSinEspacio(int * indice_tabla_asignaciones, int bloque_inicial);
void LimpiaBitEnBitMap(int offset);
bool CompararNombres(unsigned char * nom1, char * nom2);

// Semaforos
void InicializarSemaforos();

void sComenzarLecturaTablaArchivos();
void sFinalizarLecturaTablaArchivos();
void sComenzarEscrituraTablaArchivos();
void sFinalizarEscrituraTablaArchivos();

void sComenzarLecturaBitmap();
void sFinalizarLecturaBitMap();
void sComenzarEscrituraBitMap();
void sFinalizarEscrituraBitMap();

void sComenzarLecturaArchivo(int idArchivo);
void sFinalizarLecturaArchivo(int idArchivo);
void sComenzarEscrituraArchivo(int idArchivo);
void sFinalizarEscrituraArchivo(int idArchivo);


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
#define NO_HAY_ESPACIO_BITMAP -2
#define TAMANIO_MAXIMO_NOMBRE_ARCHIVO 17
#define CONTAR_TODOS_LOS_BLOQUES -1
#define SIN_BLOQUES_ASIGNADOS 0xFFFFFFFF
#define FIN_DE_DATOS_DE_ARCHIVO 0xFFFFFFFF
#define LECTURA 1
#define ESCRITURA 2
#define NO_HAY_MAS_BLOQUES_POR_RW 0
#define POKEDEX_SERVIDOR_PUERTO_ESCUCHA "POKEDEX_SERVIDOR_PUERTO_ESCUCHA"
#define NOMBRE_DISCO_OSADA "NOMBRE_DISCO_OSADA"


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

// Semaforos
//pthread_mutex_t mutex_pedido_bloques;
//sem_t * sem_tabla_archivos;
//pthread_mutex_t * mutex_escrituras;

// Tabla de archivos
pthread_mutex_t mutex_escritura_tabla_archivos;
pthread_mutex_t mutex_cuenta_lectores_tabla_archivos;
int cuenta_lectores_tabla_archivos;

// Bitmap
pthread_mutex_t mutex_escritura_bitmap;
pthread_mutex_t mutex_cuenta_lectores_bitmap;
int cuenta_lectores_bitmap;
int ultimo_bloque_disponible_encontrado;

//Archivos
pthread_mutex_t * mutex_escritura_archivos;
pthread_mutex_t * mutex_cuenta_lectores_archivos;
int * cuenta_lectores_archivos;




#endif /* POKEDEXSERVIDOR_H_ */
