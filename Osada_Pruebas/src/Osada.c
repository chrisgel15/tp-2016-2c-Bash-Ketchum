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
void ImprimirBitMap(t_bitarray *);
void ImprimirTablaDeArchivos();
bool FindDirectoryByNameAndParent(char ** path, int parentId, int * directoryId);
bool FindDirectoryByName(char * path, int * directoryId);
void SetAttrByDirectoryId(int directoryId, struct stat *stbuf);
void FindAllFilesByParentId(int * parentId, void *buf, fuse_fill_dir_t filler);
int ReadBytesFromOffset(off_t offset_from, size_t bytes_to_read, int directoryId, char * buf);

struct stat osadaStat;
int* pmap_osada;
t_log * osada_log;
int fd_osadaDisk;
int tamanio_tabla_asignaciones_bloques;

// Punteros a las diferentes estructuras del disco.
osada_header* header;
t_bitarray * bitmap;
t_bitarray * indice_bitmap_tabla_datos;
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

static int osada_read(const char *path, char *buf, size_t size, off_t offset,struct fuse_file_info *fi)
{
	int * directoryId = malloc(sizeof(char)*4);
	*directoryId = -1;

	FindDirectoryByName(path , directoryId);

	return ReadBytesFromOffset(offset, size, *directoryId, buf);
}

static int osada_write(int filedes, const void * buffer, size_t size)
{

}

static int osada_mkdir(const char* filename, mode_t mode){
/*
	char** path;
	char* parentfile ="";
	char* file="";
	u_int pos = 0;

	osada_file *tabla_archivos_aux = tabla_archivos;
	osada_file *tabla_archivos_pathaux = tabla_archivos;

	path = string_n_split(string_reverse(filename),2,"/");
	file = string_reverse(path[0]);

	if(*(path + 1)){
		parentfile = string_substring_from(string_reverse(*(path+1)),1);
	} else {
		pos = 0xFF;
	}

	while(pos != 0xFF && tabla_archivos_pathaux->state != 2 && strcmp(tabla_archivos_pathaux->fname,parentfile)==0){
		++tabla_archivos_pathaux;
		pos++;
	}

	tabla_archivos_aux = tabla_archivos;
	while(tabla_archivos_aux->state != 0){
		++tabla_archivos_aux;
	}

	tabla_archivos_aux->state = 2;
	tabla_archivos_aux->parent_directory = pos; //posicion en la que se encuentra el directorio padre en el vector.
	tabla_archivos_aux->file_size = 0;
	tabla_archivos_aux->lastmod = mode;
	tabla_archivos_aux->first_block = 0; //No tiene porque creo un directorio y no un archivo
	strcpy(tabla_archivos_aux->fname, file); //Calcular en base al filename
*/

	return 0;
}

int osada_rmdir (const char* filename){

	char ** path;
	char * directory;

	osada_file *tabla_archivos_aux = tabla_archivos;

	path = string_n_split(string_reverse(filename),2,"/");
	directory = string_reverse(path[0]);

	while(strcmp(tabla_archivos_aux->fname,directory)!=0){
		++tabla_archivos_aux;
	}

	tabla_archivos_aux->state=0;

	return 0;
}

int osada_unlink(const char* filename){

	int * directoryId = malloc(sizeof(char)*4);
	int cantBloques = 0, i = 0;

	*directoryId = -1;

	FindDirectoryByName(filename , directoryId);

	osada_file * indice_tabla_archivos = tabla_archivos;
	indice_tabla_archivos+=directoryId;

	cantBloques = TamanioEnBloques(indice_tabla_archivos->file_size);

	osada_block_pointer primerBloque = indice_tabla_archivos->first_block;

	int primerBitTablaAsignaciones = OSADA_HEADER_BLOCK_SIZE + OSADA_TABLA_ARCHIVOS_SIZE +

	while(i<cantBloques){

	}

	return -1;
}

static struct fuse_operations osada_oper = {
		.getattr = osada_getattr,
		.readdir = osada_readdir,
		.read = osada_read,
		.write = osada_write,
		.mkdir = osada_mkdir,
		.rmdir = osada_rmdir,
		.unlink = osada_unlink,

//		.open = hello_open,

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
	ImprimirHeader();
	ImprimirBitMap(bitmap);
//	ImprimirTablaDeArchivos();

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
int ReadBytesFromOffset(off_t offset_from, size_t bytes_to_read, int directoryId, char * buf)
{
	char * aux_buf = malloc(sizeof(char)*bytes_to_read);
	char * indice_aux_buf = aux_buf;

	// Ubico un puntero a la tabla de archivos para averiguar el primer bloque.
	osada_file * indice_tabla_archivos = tabla_archivos;
	indice_tabla_archivos+=directoryId;

	int * indice_tabla_asignaciones = tabla_asignaciones;
	char * indice_tabla_datos = tabla_datos;

	int bloque_actual = indice_tabla_archivos->first_block;


// Para no leer mas que el tamaño del archivo...
	if (bytes_to_read > (indice_tabla_archivos->file_size-offset_from))
		bytes_to_read = (indice_tabla_archivos->file_size-offset_from);

	int bytes_remaining = bytes_to_read;

	// Ubico el bloque donde comenzar a leer.
	while(offset_from > OSADA_BLOCK_SIZE)
	{
		bloque_actual = *(indice_tabla_asignaciones + bloque_actual);
		offset_from -= OSADA_BLOCK_SIZE;
	}

	// Ubico el indice de los datos en el byte a comenzar a leer.
	indice_tabla_datos += bloque_actual*OSADA_BLOCK_SIZE;
	indice_tabla_datos += offset_from;

	int bytes_a_leer = 0;

	while (bytes_remaining > 0)
	{
		if (bytes_remaining > OSADA_BLOCK_SIZE)
			bytes_a_leer = OSADA_BLOCK_SIZE - offset_from;
		else
			bytes_a_leer = bytes_remaining;

		memcpy((void *)indice_aux_buf,indice_tabla_datos,(bytes_a_leer));
		indice_aux_buf += bytes_a_leer;
		indice_tabla_datos += bytes_a_leer;
		offset_from += bytes_a_leer;
		bytes_remaining -= bytes_a_leer;

		// Si llegue al final de un bloque, muevo el puntero de la tabla de asignaciones...
		if (offset_from == (OSADA_BLOCK_SIZE))
		{
			bloque_actual = *(indice_tabla_asignaciones + bloque_actual);
			indice_tabla_datos = tabla_datos;
			indice_tabla_datos += bloque_actual*OSADA_BLOCK_SIZE;
			offset_from = 0;
		}


	}

	memcpy(buf,((char*)aux_buf),bytes_to_read);

	return bytes_to_read;
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
	bitmap = bitarray_create_with_mode((char*)header + sizeof(osada_header), (header->bitmap_blocks * OSADA_BLOCK_SIZE) * 8, LSB_FIRST);
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

	// Genero un nuevo puntero que apunta a un bitmap (lo vamos a poner al comienzo de donde empiezan
	// los datos, ya que es lo que vamos a escribir o borrar

	// Lo tengo que mover:
	// 1 bloque por el header + N bloques por el propio bitmap + 1024 por tabla archivos +
	// + "A" Bloques por tabla asignaciones

	int cantidad_bits_a_moverme = 1 + header->bitmap_blocks + 1024 + tamanio_tabla_asignaciones_bloques;

	log_trace(osada_log, "Cantidad Bits a Moverme dentro del Bitmap: %d", cantidad_bits_a_moverme);

	indice_bitmap_tabla_datos = bitarray_create_with_mode((char*)bitmap + cantidad_bits_a_moverme, (header->data_blocks), LSB_FIRST);



}


// Enviar el path sin la barra del principio...
bool FindDirectoryByName(char * path, int * directoryId)
{

	char * str = string_new();
	string_append(&str, path);
	char ** arr = string_split(str, "/");
	log_trace(osada_log, arr[0]);
	return FindDirectoryByNameAndParent(arr, 0xffff, directoryId);
}
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
		int i = 0;

		while(i < bitmap->size)
		{

			log_info(osada_log,"%d%d%d%d%d%d%d%d\t%d%d%d%d%d%d%d%d\t%d%d%d%d%d%d%d%d\t%d%d%d%d%d%d%d%d"
					,(int)bitarray_test_bit(bitmap, i),(int)bitarray_test_bit(bitmap, i+1)
					,(int)bitarray_test_bit(bitmap, i+2),(int)bitarray_test_bit(bitmap, i+3)
					,(int)bitarray_test_bit(bitmap, i+4),(int)bitarray_test_bit(bitmap, i+5)
					,(int)bitarray_test_bit(bitmap, i+6),(int)bitarray_test_bit(bitmap, i+7)
					,(int)bitarray_test_bit(bitmap, i+8),(int)bitarray_test_bit(bitmap, i+9)
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



			);
			i+=32;
		}

		log_info(osada_log, "\nTamanio BitMap: %d", bitmap->size);

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
