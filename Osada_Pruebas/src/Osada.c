/*
 ============================================================================
 Name        : Osada.c
 Author      : Federico Mandri
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>

#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <fuse.h>
#include <commons/bitarray.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/temporal.h>
#include <communications/ltnCommons.h>
#include <pthread.h>
#include <semaphore.h>

#include "Osada.h"

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

char * TipoDeArchivo (int tipo);
int TamanioEnBloques(int tamanioBytes);
void ImprimirBloquesDeTablaAsignacion(int * tabla_asignaciones, int file_size, int first_block);
void GenerarArchivo(char * indice_datos, int tamanio_en_bytes, int primer_bloque, char * nombre_archivo);
void UbicarPunteros();
void ImprimirHeader();
void ImprimirBitMap(t_bitarray *);
void ImprimirTablaDeArchivos();
bool FindDirectoryByNameAndParent(char ** path, int parentId, int * directoryId);
bool FindDirectoryByName(char * path, int * directoryId);
void SetAttrByDirectoryId(int directoryId, struct stat *stbuf);
void FindAllFilesByParentId(int * parentId, void *buf, fuse_fill_dir_t filler);
void ImprimirBloquesDelBitMap(t_bitarray * bitmap, int size, int * parcial, int * total);
int CantidadBloquesLibres(int condicion);
char ** SplitPath(char * path);
bool ExistsDirectoryByName(char * path);
bool FindParentDirectoryByName(char * path, int * directoryId);
bool TamanioNombreAdecuado(char * path);
void CrearArchivoDirectorio(char * path, int tablaArchivosId, int parentDirectoryId, int state);
int Crear(char * path, int state);
int LectoEscrituraFromOffset(off_t offset_from, size_t bytes_to_rw, int directoryId, char * buf, int operacion);
int FinalDeBloqueLectoEscritura(int * indice_tabla_asignaciones, int bloque_actual, int operacion);
int AsignarBloqueActualLectoEscritura(osada_file * indice_tabla_archivos, int operacion);
void InicializarSemaforos(void);

struct stat osadaStat;
int* pmap_osada;
t_log * osada_log;
int fd_osadaDisk;
int tamanio_tabla_asignaciones_bloques;

// Punteros a las diferentes estructuras del disco.
osada_header* header;
t_bitarray * bitmap;
osada_file * tabla_archivos;
int * tabla_asignaciones;
char * tabla_datos;
int offset_bitmap_datos; // offset dentro del bitmap que apunta al comienzo de la tabla de datos

//pthread_mutex_t mutex_escritura_disco;
pthread_mutex_t mutex_escritura_archivo[OSADA_CANTIDAD_MAXIMA_ARCHIVOS];
sem_t sem_lectura_disco;
int cantidad_lectores;

void InicializarSemaforos(){
	int i = 0;

	for(i=0;i<OSADA_CANTIDAD_MAXIMA_ARCHIVOS;i++){
		pthread_mutex_init(&(mutex_escritura_archivo[i]), NULL);
	}

}

static int osada_getattr(const char *path, struct stat *stbuf) {
	//pthread_mutex_lock(&mutex_escritura_disco);

	int res = 0;
	int * directoryId = malloc(sizeof(char)*4);
	*directoryId = -1;

	memset(stbuf, 0, sizeof(struct stat));

	if (strcmp(path, "/" )== 0)
	{
		log_trace(osada_log, "Entro a '/'");
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	}
	else
	{
		log_trace(osada_log, "Entro a 'ELSE'. Path: %s - DirectoryId: %d", path, *directoryId);

		FindDirectoryByName(path, directoryId);
		int pos = *directoryId;
		if (*directoryId < 0)
			res = -ENOENT;
		else {
			//pthread_mutex_lock(&(mutex_escritura_archivo[pos]));
			SetAttrByDirectoryId(*directoryId, stbuf);
			//pthread_mutex_unlock(&(mutex_escritura_archivo[pos]));
		}
	}

	free(directoryId);

	//pthread_mutex_unlock(&mutex_escritura_disco);
	return res;
}

static int osada_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
	//pthread_mutex_lock(&mutex_escritura_disco);
	(void) offset;
	(void) fi;
	int * directoryId = malloc(sizeof(char)*4);
	*directoryId = -1;

	if (strcmp(path, "/") == 0)
	{
		*directoryId = 0xffff;
		FindAllFilesByParentId(directoryId, buf, filler);

	}
	else
	{
		FindDirectoryByName(path, directoryId);
		FindAllFilesByParentId(directoryId, buf, filler);
	}

	// "." y ".." son entradas validas, la primera es una referencia al directorio donde estamos parados
	// y la segunda indica el directorio padre
	// TODO: Hay que dejar estos???
	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	//filler(buf, DEFAULT_FILE_NAME, NULL, 0);
	free(directoryId);
	//pthread_mutex_unlock(&mutex_escritura_disco);
	return 0;
}

static int osada_read(const char *path, char *buf, size_t size, off_t offset,struct fuse_file_info *fi)
{

	int * directoryId = malloc(sizeof(char)*4);
	*directoryId = -1;

	FindDirectoryByName(path , directoryId);
	int pos = *directoryId;

	//pthread_mutex_lock(&(mutex_escritura_archivo[pos]));
	int res = LectoEscrituraFromOffset(offset, size, *directoryId, buf, LECTURA);
	//return ReadBytesFromOffset(offset, size, *directoryId, buf);

	//pthread_mutex_unlock(&(mutex_escritura_archivo[pos]));
	free(directoryId);
	return res;
}

static int osada_truncate(const char * filename , off_t length)
{
	int * directoryId = malloc(sizeof(char)*4);
	int cantBloques = 0, status = 0;
	int nuevoTam = TamanioEnBloques(length);
	int res;

	*directoryId = -1;

	FindDirectoryByName(filename , directoryId);
	if (*directoryId < 0)
		res = -ENOENT;

	osada_file * indice_tabla_archivos = tabla_archivos;
	indice_tabla_archivos+=*directoryId;

	int archivoTam = TamanioEnBloques(indice_tabla_archivos->file_size);

	if(nuevoTam < archivoTam){
		cantBloques = TamanioEnBloques(indice_tabla_archivos->file_size);

		DeleteBlocks(cantBloques,*directoryId);

		indice_tabla_archivos->first_block = SIN_BLOQUES_ASIGNADOS;
		indice_tabla_archivos->file_size = 0;
	} else {
		// Comparo cantidad de bloques necesarios vs. Cantidad de bloques asignados
		int cantBloquesNecesarios = 0, cantBloquesAsignados = 0, cantBloquesAgregar = 0;

		cantBloquesAsignados = TamanioEnBloques(indice_tabla_archivos->file_size);
		cantBloquesNecesarios = TamanioEnBloques((int)(length));

		cantBloquesAgregar = cantBloquesNecesarios - cantBloquesAsignados;

		// Buscar si tengo esa cantidad de bloques disponibles
		if (CantidadBloquesLibres(cantBloquesAgregar) == cantBloquesAgregar)
		{
			// Agrego los bloques
			res = LectoEscrituraFromOffset(indice_tabla_archivos->file_size, length, *directoryId, "\0", ESCRITURA);
		}
		else
			res = -ENOSPC;
	}

	free(directoryId);
	return 0;
}

int Crear(char * path, int state)
{
	//pthread_mutex_lock(&mutex_escritura_disco);
	// Chequear espacio disponible
	int returnValue = -1;
	int tablaArchivosId = BuscaPrimerEspacioDisponibleEnTablaArchivos();

	if (tablaArchivosId > NO_HAY_ESPACIO_TABLA_ARCHIVOS)
	{
		// Chequear longitud del nombre
		if (TamanioNombreAdecuado(path))
		{
			// Chequear que no exista otro igual
			if (!ExistsDirectoryByName(path))
			{
				// Ubicar el ID del directorio padre
				int * parentDirectoryId = malloc(sizeof(char)*4);
				*parentDirectoryId = -1;
				if (FindParentDirectoryByName(path, parentDirectoryId))
				{
					//pthread_mutex_lock(&(mutex_escritura_archivo[tablaArchivosId]));
					// Generar la carpeta
					CrearArchivoDirectorio(path, tablaArchivosId, *parentDirectoryId, state);
					returnValue = 0;
					//pthread_mutex_unlock(&(mutex_escritura_archivo[tablaArchivosId]));
				}
				else
					returnValue = -ENOENT;

				free(parentDirectoryId);
			}
			else
				returnValue = -ENOENT;
		}
		else
			returnValue = -EFBIG;
	}
	else
		returnValue = -ENOSPC;

	//pthread_mutex_unlock(&mutex_escritura_disco);

	return returnValue;

}


int osada_rmdir (const char* filename){
	osada_file *indice_tabla_archivos = tabla_archivos;
	int * directoryId = malloc(sizeof(char)*4);

	FindDirectoryByName(filename, directoryId);

	osada_file *indice_tabla_archivos_busqueda = tabla_archivos;
	int i = 0;

	while(i < OSADA_CANTIDAD_MAXIMA_ARCHIVOS)
	{
		if ((int)indice_tabla_archivos_busqueda->state != 0)
		{
			if (indice_tabla_archivos_busqueda->parent_directory == *directoryId)
			{
				return ENOTEMPTY;
			}
		}
		indice_tabla_archivos_busqueda++;
		i++;
	}

	indice_tabla_archivos+=*directoryId;

	indice_tabla_archivos->state = 0;

	free(directoryId);
	return 0;
}

int osada_unlink(const char* filename){

	int * directoryId = malloc(sizeof(char)*4);
	int cantBloques = 0, status = 0;

	*directoryId = -1;

	FindDirectoryByName(filename , directoryId);

	osada_file * indice_tabla_archivos = tabla_archivos;
	indice_tabla_archivos+=*directoryId;

	cantBloques = TamanioEnBloques(indice_tabla_archivos->file_size);

	status = DeleteBlocks(cantBloques,*directoryId);

	if(!status){
		indice_tabla_archivos->state=0;
	}

	free(directoryId);
	return status;
}

int osada_rename(const char* oldname, const char* newname){
	int * directoryId = malloc(sizeof(char)*4);
	int returnValue = -1;
	*directoryId = -1;

	osada_file * indice_tabla_archivos = tabla_archivos;

	FindDirectoryByName(oldname,directoryId);
	indice_tabla_archivos+=*directoryId;

	char ** pathPartido = string_split(string_reverse(newname),"/");
	char* nombreNuevo = string_reverse(pathPartido[0]) + '\0';


	if(TamanioNombreAdecuado(nombreNuevo)){

		int index = 0;

		while (index < 17)
		{
			if ((nombreNuevo[index]) != '\0')
				(indice_tabla_archivos)->fname[index] = (nombreNuevo[index]);
			else
				(indice_tabla_archivos)->fname[index] = '\0';
			//(indice_tabla_archivos+tablaArchivosId)->fname[index] = *((splitPath[LongitudArray(splitPath)-1])+index);
			index++;
		}
		returnValue = 0;
	}
	else returnValue = -EFBIG;

	free(directoryId);
	return returnValue;
}

static int osada_mkdir (const char * path, mode_t mode)
{
	return Crear(path, 2);
}

static int osada_write (const char * path, const char * buf, size_t size, off_t off,
		      struct fuse_file_info * fi)
{
	//pthread_mutex_lock(&mutex_escritura_disco);
	int res = 0, pos;
	int * directoryId = malloc(sizeof(char)*4);
	*directoryId = -1;

	// Punteros necesarios
	osada_file * indice_tabla_archivos = tabla_archivos;


	// Busco en la tabla de archivos por path
	FindDirectoryByName(path, directoryId);
	if (*directoryId < 0)
		res = -ENOENT;

	pos = *directoryId;
	pthread_mutex_lock(&(mutex_escritura_archivo[pos]));
	// Apunto a la entrada de la tabla de archivos correspondiente
	indice_tabla_archivos+=*directoryId;

	// Comparo cantidad de bloques necesarios vs. Cantidad de bloques asignados
	int cantBloquesNecesarios = 0, cantBloquesAsignados = 0, cantBloquesAgregar = 0;

	cantBloquesAsignados = TamanioEnBloques(indice_tabla_archivos->file_size);
	cantBloquesNecesarios = TamanioEnBloques((int)(size + off));

	cantBloquesAgregar = cantBloquesNecesarios - cantBloquesAsignados;

	// Buscar si tengo esa cantidad de bloques disponibles
	if (CantidadBloquesLibres(cantBloquesAgregar) == cantBloquesAgregar)
	{
		// Agrego los bloques
		//res = WriteBytesFromOffset(off, size, *directoryId, buf);
		res = LectoEscrituraFromOffset(off, size, *directoryId, buf, ESCRITURA);
	}
	else
		res = -ENOSPC;

	free(directoryId);
	pthread_mutex_unlock(&(mutex_escritura_archivo[pos]));
	//pthread_mutex_unlock(&mutex_escritura_disco);
	return res;
}

static int osada_create (const char * path, mode_t mode, struct fuse_file_info * fi)
{
	return Crear(path, 1);
}

static int osada_utimens (const char * path, const struct timespec tiempo[2]){
	int res = 0, pos;
	int * directoryId = malloc(sizeof(char)*4);
	*directoryId = -1;

	// Punteros necesarios
	osada_file * indice_tabla_archivos = tabla_archivos;

	// Busco en la tabla de archivos por path
	FindDirectoryByName(path, directoryId);
	if (*directoryId < 0)
		res = -ENOENT;

	pos = *directoryId;

	// Apunto a la entrada de la tabla de archivos correspondiente
	pthread_mutex_lock(&(mutex_escritura_archivo[pos]));

	indice_tabla_archivos+=*directoryId;

	indice_tabla_archivos->lastmod = tiempo[1].tv_sec;

	free(directoryId);
	pthread_mutex_unlock(&(mutex_escritura_archivo[pos]));
	return res;
}

static struct fuse_operations osada_oper = {
		.getattr = osada_getattr,
		.readdir = osada_readdir,
		.read = osada_read,
		.write = osada_write,
		.create = osada_create,
//		.mknod = osada_mknod,
//		.open = osada_open,
//		.flush = remote_flush,
//		.release = remote_release,
//		.unlink = remote_unlink,
		.mkdir = osada_mkdir,
		.rmdir = osada_rmdir,
		.unlink = osada_unlink,
		.truncate = osada_truncate,
		.rename = osada_rename,
		.utimens = osada_utimens,
};

/** keys for FUSE_OPT_ options */
enum {
	KEY_VERSION,
	KEY_HELP,
};

static struct fuse_opt fuse_options[] = {

		// Estos son parametros por defecto que ya tiene FUSE
		FUSE_OPT_KEY("-V", KEY_VERSION),
		FUSE_OPT_KEY("--version", KEY_VERSION),
		FUSE_OPT_KEY("-h", KEY_HELP),
		FUSE_OPT_KEY("--help", KEY_HELP),
		FUSE_OPT_END,
};


int main(int argc, char *argv[]) {

	osada_log = CreacionLogWithLevel("osada-log", "osada-program", "TRACE");

	fd_osadaDisk= open("/home/utnso/osadaDisks/basic.bin",O_RDWR);
	fstat(fd_osadaDisk,&osadaStat);
	pmap_osada= mmap(0, osadaStat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_osadaDisk, 0);

	UbicarPunteros();
//	ImprimirHeader();
//	ImprimirBitMap(bitmap);
	ImprimirTablaDeArchivos();
//	CantidadBloquesLibres();
	InicializarSemaforos();

	// FUSE
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

		// Limpio la estructura que va a contener los parametros
	//	memset(&runtime_options, 0, sizeof(struct t_runtime_options));

		// Esta funcion de FUSE lee los parametros recibidos y los intepreta
		if (fuse_opt_parse(&args, NULL, fuse_options, NULL) == -1){
			/** error parsing options */
			perror("Invalid arguments!");
			return EXIT_FAILURE;
		}

		//pthread_mutex_init(&mutex_escritura_disco,NULL);
		sem_init(&sem_lectura_disco, 0, 10);

		// Esta es la funcion principal de FUSE, es la que se encarga
		// de realizar el montaje, comuniscarse con el kernel, delegar
		// en varios threads
		return fuse_main(args.argc, args.argv, &osada_oper, NULL);

	// FUSE

}



char * TipoDeArchivo (int tipo)
{
	switch(tipo)
	{
	case 0: return "Borrado";
	case 1: return "Ocupado";
	case 2: return "Directorio";
	}
}
int TamanioEnBloques(int tamanioBytes)
{
	float resto = tamanioBytes % OSADA_BLOCK_SIZE;
	float tam = tamanioBytes / OSADA_BLOCK_SIZE;
	int returnValue = 0;

	if (resto > 0)
		tam = tam + 1;

	return tam;
}


int DeleteBlocks(size_t blocks_to_delete, int directoryId){
	//pthread_mutex_lock(&mutex_escritura_disco);
	pthread_mutex_lock(&(mutex_escritura_archivo[directoryId]));
	bool algo;
	int i = 0;
	int offset_tabla_datos = 1 + header->bitmap_blocks + OSADA_TABLA_ARCHIVOS_SIZE + tamanio_tabla_asignaciones_bloques;

	t_bitarray *bitmap_aux = bitmap;

	osada_file * indice_tabla_archivos = tabla_archivos;
	indice_tabla_archivos+=directoryId;

	int * indice_tabla_asignaciones = tabla_asignaciones;

	int bloque_actual = indice_tabla_archivos->first_block;

	while(i<blocks_to_delete){
		//algo = bitarray_test_bit(bitmap_aux,offset_tabla_datos+bloque_actual);

		bitarray_clean_bit(bitmap_aux,offset_tabla_datos+bloque_actual);
		i++;
		bloque_actual = *(indice_tabla_asignaciones + bloque_actual);
	}

	pthread_mutex_unlock(&(mutex_escritura_archivo[directoryId]));
	//pthread_mutex_unlock(&mutex_escritura_disco);
	return 0;
}

int LectoEscrituraFromOffset(off_t offset_from, size_t bytes_to_rw, int directoryId, char * buf, int operacion)
{

	// Solo para la lectura
	char * aux_buf = malloc(sizeof(char) * bytes_to_rw);
	char * indice_aux_buf = aux_buf;

	// Ubico un puntero a la tabla de archivos para averiguar el primer bloque.
	osada_file * indice_tabla_archivos = tabla_archivos;
	indice_tabla_archivos += directoryId;

	int * indice_tabla_asignaciones = tabla_asignaciones;
	char * indice_tabla_datos = tabla_datos;

	int bloque_actual, bloque_anterior;

	// Ver como funciona la lectura de archivo que no tiene bloques asignados
	bloque_actual = AsignarBloqueActualLectoEscritura(indice_tabla_archivos, operacion);

	// Para no leer mas que el tamaño del archivo...
	if (operacion == LECTURA)
		if (bytes_to_rw > (indice_tabla_archivos->file_size - offset_from))
			bytes_to_rw = (indice_tabla_archivos->file_size - offset_from);

	int bytes_remaining = bytes_to_rw;

	// Ubico el bloque donde comenzar a escribir.
	while (offset_from > OSADA_BLOCK_SIZE)
	{
		bloque_actual = *(indice_tabla_asignaciones + bloque_actual);
		offset_from -= OSADA_BLOCK_SIZE;
	}

	// Ubico el indice de los datos en el byte a comenzar a leer.
	indice_tabla_datos += bloque_actual * OSADA_BLOCK_SIZE;
	indice_tabla_datos += offset_from;

	int bytes_a_rw = 0;

	while (bytes_remaining > 0)
	{
		if (bytes_remaining > OSADA_BLOCK_SIZE)
			bytes_a_rw = OSADA_BLOCK_SIZE - offset_from;
		else
			bytes_a_rw = bytes_remaining;

		if (operacion == ESCRITURA)
		{
			memcpy((void *)indice_tabla_datos, buf, (bytes_a_rw));
			buf += bytes_a_rw;
		}
		else
		{
			memcpy((void *)indice_aux_buf, indice_tabla_datos, (bytes_a_rw));
			indice_aux_buf += bytes_a_rw;
		}
		indice_tabla_datos += bytes_a_rw;
		offset_from += bytes_a_rw;
		bytes_remaining -= bytes_a_rw;

		// Si llegue al final de un bloque, muevo el puntero de la tabla de asignaciones...
		if (offset_from == (OSADA_BLOCK_SIZE) && bytes_remaining != NO_HAY_MAS_BLOQUES_POR_RW)
		{
			bloque_actual = FinalDeBloqueLectoEscritura(indice_tabla_asignaciones, bloque_actual, operacion);

			indice_tabla_datos = tabla_datos;
			indice_tabla_datos += bloque_actual * OSADA_BLOCK_SIZE;
			offset_from = 0;
		}

	}

	if (operacion == ESCRITURA)
	{
		// Marco el fin de los bloques.
		*(indice_tabla_asignaciones + bloque_actual) = 0xFFFFFFFF;
		indice_tabla_archivos->file_size += bytes_to_rw;
	}
	else
	{
		memcpy(buf, ((char*)aux_buf), bytes_to_rw);
	}

	free(aux_buf);
	return bytes_to_rw;
}

int AsignarBloqueActualLectoEscritura(osada_file * indice_tabla_archivos, int operacion)
{
	int aux_bloque_actual;

	if (indice_tabla_archivos->first_block == SIN_BLOQUES_ASIGNADOS)
	{
		// No tiene bloque inicial, asignarlo. // Solo para escritura
		if (operacion == ESCRITURA)
		{
			aux_bloque_actual = BuscaPrimerEspacioDisponibleEnBitMap();
			indice_tabla_archivos->first_block = aux_bloque_actual;
			SeteaBitEnBitMap(aux_bloque_actual);
		}
	}
	else
	{
		aux_bloque_actual = indice_tabla_archivos->first_block;
	}

	return aux_bloque_actual;
}

int FinalDeBloqueLectoEscritura(int * indice_tabla_asignaciones, int bloque_actual, int operacion)
{
	int bloque_anterior;

	if (operacion == ESCRITURA)
	{
		bloque_anterior = bloque_actual;
		bloque_actual = BuscaPrimerEspacioDisponibleEnBitMap();
		SeteaBitEnBitMap(bloque_actual);
		*(indice_tabla_asignaciones + bloque_anterior) = bloque_actual;
	}
	else
	{
		bloque_actual = *(indice_tabla_asignaciones + bloque_actual);
	}

	return bloque_actual; // Es un nuevo bloque Actual

}

void UbicarPunteros()
{
	// header 1 bloque - F = Total Amount of Block (fs_blocks)
	// BitMap: N bloques = F / 8 / BLOCK_SIZE
	// Tabla Archivos: 2048 bloques (fijo)
	// Tabla de asignaciones (A): (F - 1 - N - 2048) * 4 / BLOCK_SIZE
	// Bloques de Datos: F - 1 - N - 1024 - A

	// Puntero al header
	header = (osada_header *)pmap_osada;

	// Me muevo un bloque y apunto al bitmap...
	bitmap = bitarray_create_with_mode((char*)header + sizeof(osada_header), (header->bitmap_blocks * OSADA_BLOCK_SIZE) * 8, LSB_FIRST);
	t_bitarray * indice_bitmap = bitmap;

	// Me muevo un bloque y la cantidad de bloques que ocupe el bitmap (apunto a la tabla de archivos)
	tabla_archivos = header + 1 + header->bitmap_blocks;

	// Me muevo la cantidad de bloques de la tabla de archivos (1024) y apunto a la tabla de asignaciones.
	tabla_asignaciones = header + 1 + header->bitmap_blocks + OSADA_TABLA_ARCHIVOS_SIZE;

	// Tamanio tabla de asignacion (Calculado)
	tamanio_tabla_asignaciones_bloques = TamanioEnBloques(((header->fs_blocks) - OSADA_HEADER_BLOCK_SIZE - (header->bitmap_blocks)
					- OSADA_TABLA_ARCHIVOS_SIZE) * 4);


	// Me muevo el tamaño de la tabla de asignaciones y apunto a la tabla de datos...
	tabla_datos = header + 1 + header->bitmap_blocks + OSADA_TABLA_ARCHIVOS_SIZE + tamanio_tabla_asignaciones_bloques;

	// offset dentro del bitmap para apuntar a donde comienzan los bloques de datos.
	offset_bitmap_datos = 1 + header->bitmap_blocks + OSADA_TABLA_ARCHIVOS_SIZE + tamanio_tabla_asignaciones_bloques;

}
// Enviar el path sin la barra del principio...
bool FindDirectoryByName(char * path, int * directoryId)
{
	char ** arr = SplitPath(path);
	return FindDirectoryByNameAndParent(arr, 0xffff, directoryId);
}

bool ExistsDirectoryByName(char * path)
{
	char ** arr = SplitPath(path);
	int * directoryId = malloc(sizeof(char)*4);
	*directoryId = -1;
	bool res = FindDirectoryByNameAndParent(arr, 0xffff, directoryId);

	free(directoryId);

	return res;
}


char ** SplitPath(char * path)
{
	char * str = string_new();
	string_append(&str, path);
	char ** arr = string_split(str, "/");
//	log_trace(osada_log, arr[0]);
	return arr;
}

bool FindParentDirectoryByName(char * path, int * directoryId)
{
	char ** arr = SplitPath(path);
	int lArray = LongitudArray(arr);

	if (lArray == 1) return 0xffff; // Es un directorio en el directorio raiz.

	arr[lArray - 1] = NULL;
	log_trace(osada_log, arr[0]);
	return FindDirectoryByNameAndParent(arr, 0xffff, directoryId);
}

int LongitudArray(char ** arr)
{
	int index = 0;

	while(arr[index] != NULL)
		index++;

	return index;
}

bool FindDirectoryByNameAndParent(char ** path, int  parentId, int * directoryId)
{
	bool exists = false;
	osada_file * indice_tabla_archivos = tabla_archivos;
	int i = 0;



	while(i < OSADA_CANTIDAD_MAXIMA_ARCHIVOS && !exists)
	{
		if ((int)indice_tabla_archivos->state != 0)
		{
			if (memcmp(indice_tabla_archivos->fname,path[0],strlen(path[0])) == 0)
			{
//			log_trace(osada_log, "indice_tabla_archivos->fname: %s", indice_tabla_archivos->fname);
//			log_trace(osada_log, "Longitud: %d", strlen(indice_tabla_archivos->fname));
//			log_trace(osada_log, "path[0]: %s", path[0]);
//			log_trace(osada_log, "Longitud: %d", strlen(path[0]));
//			log_trace(osada_log, "Son Iguales?: %d\n", memcmp(indice_tabla_archivos->fname,path[0],strlen(path[0])));

			}
//			log_trace(osada_log, "Estoy Buscando el archivo: %s - Que tiene Id: %d y padre: %04x", *path, *directoryId, parentId);
//			log_trace(osada_log, "File Name: %s - Parent_directory: %d - %04x", indice_tabla_archivos->fname, indice_tabla_archivos->parent_directory, indice_tabla_archivos->parent_directory);
			//if (indice_tabla_archivos->parent_directory == parentId && strcmp(indice_tabla_archivos->fname,path[0]) == 0)
			if (indice_tabla_archivos->parent_directory == parentId && memcmp(indice_tabla_archivos->fname,path[0],strlen(path[0])) == 0)
			{
				if (path[1] != NULL)
					return FindDirectoryByNameAndParent(&path[1], (int)(indice_tabla_archivos - tabla_archivos), directoryId);
				else
				{
					exists = true;
					*directoryId = (int)(indice_tabla_archivos - tabla_archivos);
					log_trace(osada_log, "Encontrado!!!");
				}
			}
		}
		indice_tabla_archivos++;
		i++;
	}

	return exists;

}

void FindAllFilesByParentId(int * parentId, void *buf, fuse_fill_dir_t filler)
{
	osada_file * indice_tabla_archivos = tabla_archivos;
	int i = 0;

		while(i < OSADA_CANTIDAD_MAXIMA_ARCHIVOS)
		{
			if ((int)indice_tabla_archivos->state != 0)
			{
				if (indice_tabla_archivos->parent_directory == *parentId)
				{
					char * name = malloc(OSADA_FILENAME_LENGTH+1*sizeof(char));
					strcpy(name, indice_tabla_archivos->fname);
					*(name+OSADA_FILENAME_LENGTH) = '\0';
					filler(buf, name, NULL, 0);
					//filler(buf, "asdfgasdfgasdfgas", NULL, 0);
					free(name);
				}
			}
			indice_tabla_archivos++;
			i++;
		}
}

void SetAttrByDirectoryId(int directoryId, struct stat *stbuf)
{
	osada_file * indice_tabla_archivos = tabla_archivos;
	indice_tabla_archivos+=directoryId;

	bool esArchivo = strcmp(TipoDeArchivo(indice_tabla_archivos->state),"Ocupado") == 0;
	log_trace(osada_log, "%s esArchivo %d", indice_tabla_archivos->fname, esArchivo);

	stbuf->st_mode = esArchivo == 1 ? S_IFREG | 0444 : S_IFDIR | 0755;
	stbuf->st_nlink = 1;
	stbuf->st_size = indice_tabla_archivos->file_size;
	stbuf->st_mtim.tv_sec = indice_tabla_archivos->lastmod;
}

int CantidadBloquesLibres(int condicion)
{
	int offset_tabla_datos = 1 + header->bitmap_blocks + OSADA_TABLA_ARCHIVOS_SIZE + tamanio_tabla_asignaciones_bloques;
	int bloques_libres = 0;
	int index = 0;

	while(index < header->data_blocks &&
			(condicion == CONTAR_TODOS_LOS_BLOQUES || condicion > bloques_libres ))
	{
		if (!(int)bitarray_test_bit(bitmap, offset_tabla_datos))
			bloques_libres++;

		offset_tabla_datos++;
		index++;
	}

	log_info(osada_log, "Cantidad de Bloques libres: %d", bloques_libres);

	return bloques_libres;

}

int BuscaPrimerEspacioDisponibleEnTablaArchivos()
{
	osada_file * indice_tabla_archivos = tabla_archivos;
	int index = 0;
	bool hayEspacio = false;

	while(!hayEspacio && index < OSADA_CANTIDAD_MAXIMA_ARCHIVOS)
	{
		if ((indice_tabla_archivos+index)->state == 0)
			hayEspacio = true;
		else
			index++;
	}

	return hayEspacio ? index : NO_HAY_ESPACIO_TABLA_ARCHIVOS;

}

int BuscaPrimerEspacioDisponibleEnBitMap()
{
	int index = 0;
	bool hayEspacio = false;
	t_bitarray * bm = bitmap;

	while(!hayEspacio && index < header->data_blocks)
	{
		int bit_value = (int)bitarray_test_bit(bm, offset_bitmap_datos + index);

		if (bit_value == 0)
			hayEspacio = true;
		else
			index++;
	}

	return hayEspacio ? index : NO_HAY_ESPACIO_BITMAP;
}

void SeteaBitEnBitMap(int offset)
{
	t_bitarray * bm = bitmap;
	bitarray_set_bit(bm, offset_bitmap_datos + offset);
}

bool TamanioNombreAdecuado(char * path)
{
	char ** splitPath = SplitPath(path);

	return (strlen(splitPath[LongitudArray(splitPath)-1]) <= TAMANIO_MAXIMO_NOMBRE_ARCHIVO);

}

void CrearArchivoDirectorio(char * path, int tablaArchivosId, int parentDirectoryId, int state)
{
	log_trace(osada_log, "Comienzo a Crear el directorio...%s", path);
	osada_file * indice_tabla_archivos = tabla_archivos;

	char ** splitPath = SplitPath(path);

	int name = 0;
	while(splitPath[name] != NULL)
	{
		log_trace(osada_log, "nombre PARCIAL: %s", splitPath[name]);
		name++;
	}
	char * nombre_archivo = splitPath[name-1];

	log_trace(osada_log, "nombre: %s", nombre_archivo);

	int index = 0;

//	while(index < 17)
//	{
//		log_trace(osada_log, "letra %d: %c", index, *(nombre_archivo+index));
//		index++;
//	}
//	index = 0;

	while (index < 17)
	{
		if ((nombre_archivo[index]) != '\0')
			(indice_tabla_archivos+tablaArchivosId)->fname[index] = (nombre_archivo[index]);
		else
			(indice_tabla_archivos+tablaArchivosId)->fname[index] = '\0';
		//(indice_tabla_archivos+tablaArchivosId)->fname[index] = *((splitPath[LongitudArray(splitPath)-1])+index);
		index++;
	}

	if (parentDirectoryId == -1)
		(indice_tabla_archivos+tablaArchivosId)->parent_directory = (uint16_t)0xffff;
	else
		(indice_tabla_archivos+tablaArchivosId)->parent_directory = parentDirectoryId;
	(indice_tabla_archivos+tablaArchivosId)->state = state;




	(indice_tabla_archivos+tablaArchivosId)->file_size = 0;

	(indice_tabla_archivos+tablaArchivosId)->first_block = SIN_BLOQUES_ASIGNADOS; // todo: check!
//
//	log_trace(osada_log, "Fin creacion del directorio...%s - con padre id: %04x", path, (indice_tabla_archivos+tablaArchivosId)->parent_directory);
//	if (parentDirectoryId == -1)
//	log_trace(osada_log, "Padre Original: %04x - Padre Guardado: %04x", 0xffff, (indice_tabla_archivos+tablaArchivosId)->parent_directory);
//	else
//		log_trace(osada_log, "Padre Original: %04x - Padre Guardado: %04x", parentDirectoryId, (indice_tabla_archivos+tablaArchivosId)->parent_directory);
//
//	log_trace(osada_log, "File Size: %d", (indice_tabla_archivos+tablaArchivosId)->file_size);
}

// Metodos para Test
void ImprimirHeader()
{

	log_info(osada_log, "El filesystem es: %s", header->magic_number);
	log_info(osada_log, "La version del FS es: %d", header->version);

	// total amount of blocks
	log_info(osada_log,"El tamanio en bloques del FS es: %d", header->fs_blocks); // "T / BLOCK_SIZE"

	// bitmap size in blocks
	log_info(osada_log,"El tamanio en bloques del bitmap del FS es: %d",header->bitmap_blocks);

	// allocations table's first block number
	log_info(osada_log,"El primer bloque de la tabla de asignacion del FS es: %d",header->allocations_table_offset);

	// amount of data blocks
	log_info(osada_log,"La cantidad del bloques para datos del FS es: %d",header->data_blocks);

	log_info(osada_log,"El tamanio de la tabla de asignacion es (CALCULADO): %d",tamanio_tabla_asignaciones_bloques);


}
void ImprimirBloquesDeTablaAsignacion(int * tabla_asignaciones, int file_size, int first_block)
{
	int tamanioEnBloques = TamanioEnBloques(file_size);
	int * coleccion = malloc(sizeof(int)*tamanioEnBloques);

	log_info(osada_log , "%d", first_block);
	int i = 1;
	int proxima_posicion = first_block;

	while(i < tamanioEnBloques)
	{
		log_info(osada_log , "%d", *(tabla_asignaciones+proxima_posicion));
		*(coleccion+i) = *(tabla_asignaciones+proxima_posicion);
		proxima_posicion = *(tabla_asignaciones+proxima_posicion);
		i++;
	}

	free(coleccion);

}
void GenerarArchivo(char * indice_datos, int tamanio_en_bytes, int primer_bloque, char * nombre_archivo)
{
	char * path = string_new();
	string_append(&path,"/home/utnso/archivosChallenge/");
	string_append(&path,nombre_archivo);
	string_append(&path,"-");
	string_append(&path,temporal_get_string_time());

	char * archivo = malloc(sizeof(char)*tamanio_en_bytes);
	int tamanio_en_bloques = TamanioEnBloques(tamanio_en_bytes);
	int k = 0;
	int bloque_actual = primer_bloque;

	FILE *fp;
	fp = fopen( path , "w" );
	while( k < tamanio_en_bloques)
	{
		int bytes_restantes = (tamanio_en_bytes - k*64);
		int cantidad_bytes = bytes_restantes >= 64 ? 64 : bytes_restantes;
		fwrite((void *)(indice_datos + OSADA_BLOCK_SIZE*bloque_actual)  , cantidad_bytes , 1, fp );
		k++;
		bloque_actual = (int)*(tabla_asignaciones + bloque_actual);
	}

   free(archivo);

   fclose(fp);

}
void ImprimirBitMap(t_bitarray * bitmap)
{
		int * total = malloc(sizeof(int));
		int * parcial = malloc(sizeof(int));
		*total = 0;
		*parcial = 0;

		log_info(osada_log, "\nTamanio BitMap: %d", bitmap->size);

		/*Impresion del bit de header*/
		log_trace(osada_log, "Impresion del bit del header");
		ImprimirBloquesDelBitMap(bitmap, 1, parcial, total);

		/*Impresion de los bits del bitmap*/
		*parcial = 0;
		log_trace(osada_log, "Impresion de los bits del BitMap");
		ImprimirBloquesDelBitMap(bitmap, header->bitmap_blocks, parcial, total);

		/*Impresion de tabla de archivos*/
		log_trace(osada_log, "Impresion de los bits de la tabla de archivos");
		*parcial = 0;
		ImprimirBloquesDelBitMap(bitmap, OSADA_TABLA_ARCHIVOS_SIZE, parcial, total);

		/*Impresion de bits de la tabla de asignaciones*/
		//j = 0;
		*parcial = 0;
		log_trace(osada_log, "Impresion de los bits de la tabla de asignaciones");
		ImprimirBloquesDelBitMap(bitmap, tamanio_tabla_asignaciones_bloques, parcial, total);

		/*Impresion de bits de la tabla de datos*/
		*parcial = 0;
		log_trace(osada_log, "Impresion de los bits de la tabla de datos");
		ImprimirBloquesDelBitMap(bitmap, header->data_blocks, parcial, total);
}

void ImprimirTablaDeArchivos()
{
	int j = 0;

	//Indices para recorrer sin modificar los originales...
	osada_file * indice_tabla_archivos = tabla_archivos;
	char * indice_datos = tabla_datos;

	for (j = 0 ; j < OSADA_CANTIDAD_MAXIMA_ARCHIVOS ; j++)
	{
		//if ((int)indice_tabla_archivos->state != 0)
		{
			log_info(osada_log, " ---- Archivo %d ---- ", j);
			log_info(osada_log, "Estado: %s", TipoDeArchivo((int)indice_tabla_archivos->state));
			log_info(osada_log, "Nombre: %s", indice_tabla_archivos->fname);
			log_info(osada_log, "Tamanio: %d bytes - %d bloques", indice_tabla_archivos->file_size, TamanioEnBloques(indice_tabla_archivos->file_size));
			log_info(osada_log, "Directorio Padre: %04x hexa - %d decimal", indice_tabla_archivos->parent_directory, indice_tabla_archivos->parent_directory);
			log_info(osada_log, "Primer Bloque: %04x hexa - %d decimal", indice_tabla_archivos->first_block, indice_tabla_archivos->first_block);
			//ImprimirBloquesDeTablaAsignacion(tabla_asignaciones, indice_tabla_archivos->file_size, indice_tabla_archivos->first_block);
			log_info(osada_log, "Fecha Ultima Modificacion: %d\n", indice_tabla_archivos->lastmod);
			//GenerarArchivo(indice_datos, indice_tabla_archivos->file_size, indice_tabla_archivos->first_block, &(indice_tabla_archivos->fname));
		}
		indice_tabla_archivos++;
	}
}

void ImprimirUnBloqueDeBits(t_bitarray * bitmap, int * total, int * parcial)
{
	int i = *total;
	log_info(osada_log,"%d%d%d%d%d%d%d%d  %d%d%d%d%d%d%d%d  %d%d%d%d%d%d%d%d  %d%d%d%d%d%d%d%d  "
			"%d%d%d%d%d%d%d%d  %d%d%d%d%d%d%d%d  %d%d%d%d%d%d%d%d  %d%d%d%d%d%d%d%d Parcial:%d Total:%d"
						,(int)bitarray_test_bit(bitmap, i),(int)bitarray_test_bit(bitmap, i+1)
						,(int)bitarray_test_bit(bitmap, i+2),(int)bitarray_test_bit(bitmap, i+3)
						,(int)bitarray_test_bit(bitmap, i+4),(int)bitarray_test_bit(bitmap, i+5)
						,(int)bitarray_test_bit(bitmap, i+6),(int)bitarray_test_bit(bitmap, i+7)
						,(int)bitarray_test_bit(bitmap, i+8),(int)bitarray_test_bit(bitmap, i+1)
						,(int)bitarray_test_bit(bitmap, i+10),(int)bitarray_test_bit(bitmap, i+11)
						,(int)bitarray_test_bit(bitmap, i+12),(int)bitarray_test_bit(bitmap, i+13)
						,(int)bitarray_test_bit(bitmap, i+14),(int)bitarray_test_bit(bitmap, i+15)
						,(int)bitarray_test_bit(bitmap, i+16),(int)bitarray_test_bit(bitmap, i+17)
						,(int)bitarray_test_bit(bitmap, i+18),(int)bitarray_test_bit(bitmap, i+19)
						,(int)bitarray_test_bit(bitmap, i+20),(int)bitarray_test_bit(bitmap, i+21)
						,(int)bitarray_test_bit(bitmap, i+22),(int)bitarray_test_bit(bitmap, i+23)
						,(int)bitarray_test_bit(bitmap, i+24),(int)bitarray_test_bit(bitmap, i+25)
						,(int)bitarray_test_bit(bitmap, i+26),(int)bitarray_test_bit(bitmap, i+27)
						,(int)bitarray_test_bit(bitmap, i+28),(int)bitarray_test_bit(bitmap, i+29)
						,(int)bitarray_test_bit(bitmap, i+30),(int)bitarray_test_bit(bitmap, i+31)
						,(int)bitarray_test_bit(bitmap, i+32),(int)bitarray_test_bit(bitmap, i+33)
						,(int)bitarray_test_bit(bitmap, i+34),(int)bitarray_test_bit(bitmap, i+35)
						,(int)bitarray_test_bit(bitmap, i+36),(int)bitarray_test_bit(bitmap, i+37)
						,(int)bitarray_test_bit(bitmap, i+38),(int)bitarray_test_bit(bitmap, i+39)
						,(int)bitarray_test_bit(bitmap, i+40),(int)bitarray_test_bit(bitmap, i+41)
						,(int)bitarray_test_bit(bitmap, i+42),(int)bitarray_test_bit(bitmap, i+43)
						,(int)bitarray_test_bit(bitmap, i+44),(int)bitarray_test_bit(bitmap, i+45)
						,(int)bitarray_test_bit(bitmap, i+46),(int)bitarray_test_bit(bitmap, i+47)
						,(int)bitarray_test_bit(bitmap, i+48),(int)bitarray_test_bit(bitmap, i+49)
						,(int)bitarray_test_bit(bitmap, i+50),(int)bitarray_test_bit(bitmap, i+51)
						,(int)bitarray_test_bit(bitmap, i+52),(int)bitarray_test_bit(bitmap, i+53)
						,(int)bitarray_test_bit(bitmap, i+54),(int)bitarray_test_bit(bitmap, i+55)
						,(int)bitarray_test_bit(bitmap, i+56),(int)bitarray_test_bit(bitmap, i+57)
						,(int)bitarray_test_bit(bitmap, i+58),(int)bitarray_test_bit(bitmap, i+59)
						,(int)bitarray_test_bit(bitmap, i+60),(int)bitarray_test_bit(bitmap, i+61)
						,(int)bitarray_test_bit(bitmap, i+62),(int)bitarray_test_bit(bitmap, i+63)
						,(*parcial)+64
						,(*total)+64
						);
	*total+=64;
	*parcial+=64;
}

void ImprimirBloquesDelBitMap(t_bitarray * bitmap, int size, int * parcial, int * total)
{
	while(*parcial < size){
		if ((int)(size - *parcial) >= 64)
			ImprimirUnBloqueDeBits(bitmap, total, parcial);
		else{
			log_trace(osada_log, "Impresion de a uno...");
			while((int)(size - *parcial) > 0){
				*parcial+=1;
				*total+=1;
				log_trace(osada_log,"%d Parcial:%d Total:%d",(int)bitarray_test_bit(bitmap, *total),(*parcial),(*total));
			}
		}
	}
}


