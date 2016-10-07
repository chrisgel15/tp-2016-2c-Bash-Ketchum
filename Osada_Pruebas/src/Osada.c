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
#include <communications/ltnCommons.h>

#include "Osada.h"

#define OSADA_BLOCK_SIZE 64
#define OSADA_TABLA_ARCHIVOS_SIZE 1024
#define OSADA_HEADER_BLOCK_SIZE 1

char * TipoDeArchivo (int tipo);
int TamanioEnBloques(int tamanioBytes);
void ImprimirBloquesDeTablaAsignacion(int * tabla_asignaciones, int file_size, int first_block);
void GenerarArchivo(char * indice_datos, int tamanio_en_bytes, int primer_bloque, char * nombre_archivo);
void UbicarPunteros();
void ImprimirHeader();
void ImprimirBitMap();
void ImprimirTablaDeArchivos();
bool FindDirectoryByNameAndParent(char ** path, int parentId, int * directoryId);
bool FindDirectoryByName(char * path, int * directoryId);
void SetAttrByDirectoryId(int directoryId, struct stat *stbuf);
void FindAllFilesByParentId(int * parentId, void *buf, fuse_fill_dir_t filler);

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



static int osada_getattr(const char *path, struct stat *stbuf) {
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
		log_trace(osada_log, "Entro a 'ELSE'. Path: %s", path);

		FindDirectoryByName(path, directoryId);
		if (*directoryId < 0)
			res = -ENOENT;
		else
			SetAttrByDirectoryId(*directoryId, stbuf);
	}


	return res;
}


static int osada_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
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

	return 0;
}

static struct fuse_operations osada_oper = {
		.getattr = osada_getattr,
		.readdir = osada_readdir
//		.open = hello_open,
//		.read = hello_read,
};

/** keys for FUSE_OPT_ options */
enum {
	KEY_VERSION,
	KEY_HELP,
};

static struct fuse_opt fuse_options[] = {
		// Este es un parametro definido por nosotros
	//	CUSTOM_FUSE_OPT_KEY("--welcome-msg %s", welcome_msg, 0),

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
//	ImprimirBitMap();
//	ImprimirTablaDeArchivos();

	// Test...
	char * testStr = "/Vermilion City/Pokemons/001.txt";
	char * str = string_new();
	string_append(&str, testStr);
	char ** arr = string_split(str, "/");

	int * directoryId = malloc(sizeof(char)*4);
	*directoryId = -1;

	//bool exists = FindDirectoryByName(arr, directoryId);

	FindDirectoryByName("/.Trash", directoryId);

	// Test...

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

		// Si se paso el parametro --welcome-msg
		// el campo welcome_msg deberia tener el
		// valor pasado
//		if( runtime_options.welcome_msg != NULL ){
//			printf("%s\n", runtime_options.welcome_msg);
//		}

		// Esta es la funcion principal de FUSE, es la que se encarga
		// de realizar el montaje, comuniscarse con el kernel, delegar todo
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

void UbicarPunteros()
{
	// header 1 bloque - F = Total Amount of Block (fs_blocks)
	// BitMap: N bloques = F / 8 / BLOCK_SIZE
	// Tabla Archivos: 1024 bloques (fijo)
	// Tabla de asignaciones (A): (F - 1 - N - 1024) * 4 / BLOCK_SIZE
	// Bloques de Datos: F - 1 - N - 1024 - A

	// Puntero al header
	header = (osada_header *)pmap_osada;

	// Me muevo un bloque y apunto al bitmap...
	bitmap = bitarray_create((char*)header + sizeof(osada_header), (header->bitmap_blocks * OSADA_BLOCK_SIZE) * 8);
	t_bitarray * indice_bitmap = bitmap;

	// Me muevo un bloque y la cantidad de bloques que ocupe el bitmap (apunto a la tabla de archivos)
	tabla_archivos = header + 1 + header->bitmap_blocks;

	// Me muevo la cantidad de bloques de la tabla de archivos (1024) y apunto a la tabla de asignaciones.
	tabla_asignaciones = header + 1 + header->bitmap_blocks + 1024;

	// Tamanio tabla de asignacion (Calculado)
	tamanio_tabla_asignaciones_bloques = TamanioEnBloques(((header->fs_blocks) - OSADA_HEADER_BLOCK_SIZE - (header->bitmap_blocks)
					- OSADA_TABLA_ARCHIVOS_SIZE) * 4);


	// Me muevo el tamaño de la tabla de asignaciones y apunto a la tabla de datos...
	tabla_datos = header + 1 + header->bitmap_blocks + 1024 + tamanio_tabla_asignaciones_bloques;

}

void ImprimirBitMap()
{
	//	int i = 0;
	//
	//	while(i < indice_bitmap->size)
	//	{
	//
	//		log_info(osada_log,"%d%d%d%d%d%d%d%d"
	//				,(int)bitarray_test_bit(indice_bitmap, i)
	//				,(int)bitarray_test_bit(indice_bitmap, i+1)
	//				,(int)bitarray_test_bit(indice_bitmap, i+2)
	//				,(int)bitarray_test_bit(indice_bitmap, i+3)
	//				,(int)bitarray_test_bit(indice_bitmap, i+4)
	//				,(int)bitarray_test_bit(indice_bitmap, i+5)
	//				,(int)bitarray_test_bit(indice_bitmap, i+6)
	//				,(int)bitarray_test_bit(indice_bitmap, i+7));
	//		i+=8;
	//	}

}

void ImprimirTablaDeArchivos()
{
	int j = 0;

	//Indices para recorrer sin modificar los originales...
	osada_file * indice_tabla_archivos = tabla_archivos;
	char * indice_datos = tabla_datos;

	for (j = 0 ; j < OSADA_TABLA_ARCHIVOS_SIZE ; j++)
	{
		if ((int)indice_tabla_archivos->state != 0)
		{
			log_info(osada_log, " ---- Archivo %d ---- ", j);
			log_info(osada_log, "Estado: %s", TipoDeArchivo((int)indice_tabla_archivos->state));
			log_info(osada_log, "Nombre: %s", indice_tabla_archivos->fname);
			log_info(osada_log, "Tamanio: %d bytes - %d bloques", indice_tabla_archivos->file_size, TamanioEnBloques(indice_tabla_archivos->file_size));
			log_info(osada_log, "Directorio Padre: %04x hexa - %d decimal", indice_tabla_archivos->parent_directory, indice_tabla_archivos->parent_directory);
			log_info(osada_log, "Primer Bloque: %04x hexa - %d decimal", indice_tabla_archivos->first_block, indice_tabla_archivos->first_block);
		//	ImprimirBloquesDeTablaAsignacion(tabla_asignaciones, indice_tabla_archivos->file_size, indice_tabla_archivos->first_block);
			log_info(osada_log, "Fecha Ultima Modificacion: %d\n", indice_tabla_archivos->lastmod);
			GenerarArchivo(indice_datos, indice_tabla_archivos->file_size, indice_tabla_archivos->first_block, &(indice_tabla_archivos->fname));
		}
		indice_tabla_archivos++;
	}

}

// Enviar el path sin la barra del principio...
bool FindDirectoryByNameAndParent(char ** path, int parentId, int * directoryId)
{
	bool exists = false;
	osada_file * indice_tabla_archivos = tabla_archivos;
	int i = 0;

	while(i < OSADA_TABLA_ARCHIVOS_SIZE && !exists)
	{
		if ((int)indice_tabla_archivos->state != 0)
		{
			if (indice_tabla_archivos->parent_directory == parentId && strcmp(indice_tabla_archivos->fname,path[0]) == 0)
			{
				if (path[1] != NULL)
					return FindDirectoryByNameAndParent(&path[1], (int)(indice_tabla_archivos - tabla_archivos), directoryId);
				else
				{
					exists = true;
					*directoryId = (int)(indice_tabla_archivos - tabla_archivos);
				}
			}
		}
		indice_tabla_archivos++;
		i++;
	}

	return exists;

}

bool FindDirectoryByName(char * path, int * directoryId)
{

	char * str = string_new();
	string_append(&str, path);
	char ** arr = string_split(str, "/");
	log_trace(osada_log, arr[0]);
	return FindDirectoryByNameAndParent(arr, 0xffff, directoryId);
}

void FindAllFilesByParentId(int * parentId, void *buf, fuse_fill_dir_t filler)
{
	osada_file * indice_tabla_archivos = tabla_archivos;
	int i = 0;

		while(i < OSADA_TABLA_ARCHIVOS_SIZE)
		{
			if ((int)indice_tabla_archivos->state != 0)
			{
				if (indice_tabla_archivos->parent_directory == *parentId)
				{
					filler(buf, indice_tabla_archivos->fname, NULL, 0);
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
}




