/*
 ============================================================================
 Name        : PokedexServidor.c
 Author      :
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "PokedexServidor.h"

#define EXECUTION_MODE 0 // 0 = Release, 1 = Debug



int main(void) {

	/************ Creacion del Config ***********************/
		osada_server_config = creacion_config();

		if (osada_server_config->properties->elements_amount < 1) {
			perror("Error al abrir el archivo de configuracion del Pokedex Servidor");
			exit(0);
		}

	/**************** Creacion del Log ******************/

	osada_log = CreacionLogWithLevel("osada-server-log", "osada-server", "TRACE");

	fd_osadaDisk = open("/home/utnso/osadaDisks/basic.bin", O_RDWR);
	fstat(fd_osadaDisk, &osadaStat);
	pmap_osada = mmap(0, osadaStat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_osadaDisk, 0);

	UbicarPunteros();

	if (EXECUTION_MODE)
	{
		ImprimirHeader();
		ImprimirBitMap(bitmap);
		ImprimirTablaDeArchivos();
		CantidadBloquesLibres(CONTAR_TODOS_LOS_BLOQUES);
	}


	// todo: Creacion e inicializacion de semaforos

	// todo: levantarse como servidor
	ltn_sock_addinfo * ltn_puertoEscucha;
	int listenerSocket;

	char * puertoEscucha = config_get_string_value(osada_server_config , POKEDEX_SERVIDOR_PUERTO_ESCUCHA);
	ltn_puertoEscucha = createSocket(puertoEscucha);

	//myFree((void *)puertoEscucha, "puertoEscucha", osada_log);

	listenerSocket = doBind(ltn_puertoEscucha);
	listenerSocket = doListen(listenerSocket , 100);

	//free(ltn_puertoEscucha);

	// todo: quedarse a la espera de pedidos
	int fdCliente;
	while(1)
	{
		fdCliente = doAccept(listenerSocket);
		// todo: recibe un pedido -> crea un hilo
		CrearHilo(fdCliente);
	}

	config_destroy(osada_server_config);
	log_destroy(osada_log);


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


// Metodos para el manejo de peticiones y comunicaciones

void CrearHilo(int fdCliente)
{
	pthread_t estructuraThread;
	pthread_create(&estructuraThread,NULL,(void*)RecibirYProcesarPedido,fdCliente);
}

void RecibirYProcesarPedido(int fdCliente)
{
	int conexion_terminada = 0;
	int *result = (int *) myMalloc_int("RecibirYProcesarPedido - result", osada_log);
	while(conexion_terminada == 0)
	{
		int valorRecibido = 0;
		valorRecibido = recibirInt(fdCliente, result , osada_log);

		if (*result > 0) {
			switch (valorRecibido) {
			case GETATTR:
				log_trace(osada_log , "El cliente %d pidio un GETATTR.", fdCliente);
				ProcesarGetAttr(fdCliente);
				break;
			case READDIR:
				log_trace(osada_log , "El cliente %d pidio un READDIR.", fdCliente);
				ProcesarReadDir(fdCliente);
				break;
			default:
				log_error(osada_log, "Mensaje incorrecto. Cliente %", fdCliente);
				break;
			}
		}
		else
		{
			log_error(osada_log, "Error en el receive del cliente %d. Cierro la conexion.", fdCliente);
			conexion_terminada = 1;
			close(fdCliente);
		}
	}

	myFree(result, "RecibirYProcesarPedido - result", osada_log);


}

void ProcesarGetAttr(int fd)
{
	// Recibo el tamaño del path
	int * result = (int *)myMalloc_int("ProcesarGetAttr - result", osada_log);
	int tamanioPath;
	tamanioPath = recibirInt(fd , result, osada_log);

	// Recibo el path
	char * path = myMalloc_char((tamanioPath+1), "ProcesarGetAttr - path", osada_log);
	recibirMensaje(fd, path, tamanioPath, osada_log);

	int * directoryId = NULL;
	directoryId = myMalloc_int("ProcesarGetAttr - directoryId",osada_log);
	*directoryId = -1;

	FindDirectoryByName(path, directoryId);
	if (*directoryId < 0)
	{
		// Enviar codigo de archivo no encontrado
		enviarInt(fd, ARCHIVO_NO_ENCONTRADO);
	}
	else
	{
		// Enviar codigo de archivo encontrado
		enviarInt(fd, ARCHIVO_ENCONTRADO);
		SetAttrByDirectoryId(*directoryId, fd);
	}

	myFree(result, "ProcesarGetAttr - result", osada_log);
	myFree(path, "ProcesarGetAttr - path", osada_log);
	myFree(directoryId, "ProcesarGetAttr - directoryId", osada_log);
	result = NULL;
	path = NULL;
	directoryId = NULL;

}

void ProcesarReadDir(int fd)
{
		int * result = myMalloc_int("ProcesarReadDir - result", osada_log);
		int tamanioPath = 0;
		tamanioPath = recibirInt(fd, result, osada_log);

		char * path = myMalloc_char(tamanioPath + 1, "ProcesarReadDir - path", osada_log);
		recibirMensaje(fd, path, tamanioPath, osada_log);

		int * directoryId = myMalloc_int("ProcesarReadDir - directoryId", osada_log);
		*directoryId = -1;

		fuse_fill_dir_t filler;
		void * buf;

		if (strcmp(path, "/") == 0)
		{
			*directoryId = 0xffff;
			FindAllFilesByParentId(directoryId, buf, filler, fd);

		}
		else
		{
			FindDirectoryByName(path, directoryId);
			FindAllFilesByParentId(directoryId, buf, filler, fd);
		}

		free(result);
		free(path);
		free(directoryId);

}

void SetAttrByDirectoryId(int directoryId, int fd)
{
	osada_file * indice_tabla_archivos = tabla_archivos;
	indice_tabla_archivos+=directoryId;

	bool esArchivo = strcmp(TipoDeArchivo(indice_tabla_archivos->state),"Ocupado") == 0;
	log_trace(osada_log, "%s esArchivo %d", indice_tabla_archivos->fname, esArchivo);

	if (esArchivo == 1)
	{
		//stbuf->st_mode = esArchivo == 1 ? S_IFREG | 0444 : S_IFDIR | 0755;
		// Enviar ST_MODE CODE
		enviarInt(fd, ST_MODE_CODE_ES_ARCHIVO);
	}
	else
	{
		enviarInt(fd, ST_MODE_CODE_ES_DIRECTORIO);
	}

	//stbuf->st_nlink = 1;
	// Enviar el SIZE
	enviarInt(fd, indice_tabla_archivos->file_size);
	//stbuf->st_size = indice_tabla_archivos->file_size;
}

char ** SplitPath(char * path)
{
	char * str = string_new();
	string_append(&str, path);
	char ** arr = string_split(str, "/");
	myFree(str, "SplitPath - str", osada_log);
	return arr;
}

bool FindDirectoryByName(char * path, int * directoryId)
{
	char ** arr = SplitPath(path);
	//return FindDirectoryByNameAndParent(arr, 0xffff, directoryId);
	bool returnValue = FindDirectoryByNameAndParent(arr, 0xffff, directoryId);
	log_trace(osada_log, "Voy a liberar el path: %s", path);
	myFreeSplitPath(arr, "FindDirectoryByName - arr", osada_log);
	return returnValue;
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

void FindAllFilesByParentId(int * parentId, void *buf, fuse_fill_dir_t filler, int fd)
{
	osada_file * indice_tabla_archivos = tabla_archivos;
	int i = 0;

		while(i < OSADA_CANTIDAD_MAXIMA_ARCHIVOS)
		{
			if ((int)indice_tabla_archivos->state != 0)
			{
				if (indice_tabla_archivos->parent_directory == *parentId)
				{
					enviarInt(fd, CONTINUAR_READDIR);

					//char * name = malloc(OSADA_FILENAME_LENGTH+1*sizeof(char));
					char * name = myMalloc_char(OSADA_FILENAME_LENGTH+1, "FindAllFilesByParentId - name", osada_log);


					strcpy(name, indice_tabla_archivos->fname);
					*(name+OSADA_FILENAME_LENGTH) = '\0';

					enviarInt(fd, OSADA_FILENAME_LENGTH);
					//enviarMensaje(fd, name);
					enviarBytes(fd, (void *)name, OSADA_FILENAME_LENGTH+1);
					//filler(buf, name, NULL, 0);

					myFree(name, "FindAllFilesByParentId - name", osada_log);

				}
			}
			indice_tabla_archivos++;
			i++;
		}

		enviarInt(fd, FINALIZAR_READDIR);
}

// Metodos para test
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
		if ((int)indice_tabla_archivos->state != 0)
		{
			log_info(osada_log, " ---- Archivo %d ---- ", j);
			log_info(osada_log, "Estado: %s", TipoDeArchivo((int)indice_tabla_archivos->state));
			log_info(osada_log, "Nombre: %s", indice_tabla_archivos->fname);
			log_info(osada_log, "Tamanio: %d bytes - %d bloques", indice_tabla_archivos->file_size, TamanioEnBloques(indice_tabla_archivos->file_size));
			log_info(osada_log, "Directorio Padre: %04x hexa - %d decimal", indice_tabla_archivos->parent_directory, indice_tabla_archivos->parent_directory);
			log_info(osada_log, "Primer Bloque: %04x hexa - %d decimal", indice_tabla_archivos->first_block, indice_tabla_archivos->first_block);
			ImprimirBloquesDeTablaAsignacion(tabla_asignaciones, indice_tabla_archivos->file_size, indice_tabla_archivos->first_block);
			log_info(osada_log, "Fecha Ultima Modificacion: %d\n", indice_tabla_archivos->lastmod);
			GenerarArchivo(indice_datos, indice_tabla_archivos->file_size, indice_tabla_archivos->first_block, &(indice_tabla_archivos->fname));
		}
		indice_tabla_archivos++;
	}

}


void ImprimirBloquesDelBitMap(t_bitarray * bitmap, int size, int * parcial, int * total)
{
	while(*parcial < size)
			{
				if ((int)(size - *parcial) >= 64)
					ImprimirUnBloqueDeBits(bitmap, total, parcial);
				else
				{
					log_trace(osada_log, "Impresion de a uno...");
					while((int)(size - *parcial) > 0)
					{
						*parcial+=1;
						*total+=1;
						log_trace(osada_log,"%d Parcial:%d Total:%d",(int)bitarray_test_bit(bitmap, *total),(*parcial),(*total));
					}
				}

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

