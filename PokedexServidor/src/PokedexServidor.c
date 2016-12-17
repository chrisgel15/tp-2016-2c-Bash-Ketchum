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

	logLevel = config_get_string_value(osada_server_config , LOG_LEVEL);

	osada_log = CreacionLogWithLevel("osada-server.log", "osada-server", logLevel);

	char * nombreDisco = config_get_string_value(osada_server_config , NOMBRE_DISCO_OSADA);

	char * nombreDiscoCompleto = string_new();
	string_append(&nombreDiscoCompleto , "/home/utnso/");
	string_append(&nombreDiscoCompleto, nombreDisco);

	log_info(osada_log, "Se va a abrir el disco: %s. En caso de error chequear que exista.", nombreDiscoCompleto);

	fd_osadaDisk = open(nombreDiscoCompleto, O_RDWR);

	myFree(nombreDiscoCompleto, "nombreDiscoCompleto", osada_log);

	fstat(fd_osadaDisk, &osadaStat);
	pmap_osada = mmap(0, osadaStat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_osadaDisk, 0);

	UbicarPunteros();
	InicializarSemaforos();

	if (EXECUTION_MODE)
	{
	//	ImprimirHeader();
	//	ImprimirBitMap(bitmap);
	//	ImprimirTablaDeArchivos();
	//	CantidadBloquesLibres(CONTAR_TODOS_LOS_BLOQUES);
	}


	ltn_sock_addinfo * ltn_puertoEscucha;
	int listenerSocket;

	char * puertoEscucha = config_get_string_value(osada_server_config , POKEDEX_SERVIDOR_PUERTO_ESCUCHA);
	ltn_puertoEscucha = createSocket(puertoEscucha);

	//myFree((void *)puertoEscucha, "puertoEscucha", osada_log);

	listenerSocket = doBind(ltn_puertoEscucha);
	listenerSocket = doListen(listenerSocket , 100);

	//free(ltn_puertoEscucha);

	int fdCliente;
	while(1)
	{
		fdCliente = doAccept(listenerSocket);
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
	bitmap = bitarray_create_with_mode((char*)header + sizeof(osada_header), header->fs_blocks, MSB_FIRST);
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
				log_info(osada_log , "El cliente %d pidio un GETATTR.", fdCliente);
				ProcesarGetAttr(fdCliente);
				break;
			case READDIR:
				log_info(osada_log , "El cliente %d pidio un READDIR.", fdCliente);
				ProcesarReadDir(fdCliente);
				break;
			case READ:
				log_info(osada_log, "El cliente %d pidio un READ.", fdCliente);
				ProcesarRead(fdCliente);
				break;
			case MKDIR:
				log_info(osada_log, "El cliente %d pidio un MKDIR.", fdCliente);
				ProcesarMkDir(fdCliente);
				break;
			case CREATE:
				log_info(osada_log, "El cliente %d pidio un CREATE.", fdCliente);
				ProcesarCreate(fdCliente);
				break;
			case WRITE:
				log_info(osada_log, "El cliente %d pidio un WRITE.", fdCliente);
				ProcesarWrite(fdCliente);
				break;
			case UNLINK:
				log_info(osada_log, "El cliente %d pidio un UNLINK.", fdCliente);
				ProcesarUnlink(fdCliente);
				break;
			case TRUNCATE:
				log_info(osada_log, "El cliente %d pidio un TRUNCATE.", fdCliente);
				ProcesarTruncate(fdCliente);
				break;
			case RMDIR:
				log_info(osada_log,  "El cliente %d pidio un RMDIR.", fdCliente);
				ProcesarRmDir(fdCliente);
				break;
			case RENAME:
			 	log_info(osada_log,  "El cliente %d pidio un RENAME.", fdCliente);
			 	ProcesarRename(fdCliente);
			 	break;
			 case UTIME:
			 	log_info(osada_log,  "El cleinte %d pidio un UTIMENS", fdCliente);
			 	ProcesarUtimens(fdCliente);
			 	break;
			default:
				log_error(osada_log, "Mensaje incorrecto. Cliente %d", fdCliente);
				conexion_terminada = 1;
				close(fdCliente);
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

	// Envio el fin de GETATTR
	enviarInt(fd, FIN_GETATTR);

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

		// Envio el fin del readdir
		enviarInt(fd, FIN_READDIR);

}

void ProcesarRead(int fd)
{
	// Recibo el tamaño del path
	int * result = (int *)myMalloc_int("ProcesarRead - result", osada_log);
	int tamanioPath;
	tamanioPath = recibirInt(fd , result, osada_log);

	// Recibo el path
	char * path = myMalloc_char((tamanioPath+1), "ProcesarRead - path", osada_log);
	recibirMensaje(fd, path, tamanioPath, osada_log);

	// Recibir el size (entero)
	int size;
	size = recibirInt(fd , result, osada_log);

	// Recibir el offset (entero)
	int offset;
	offset = recibirInt(fd , result, osada_log);

	// Declaro mi variable para la entrada de la tabla de archivos
	int * directoryId = (int *)myMalloc_int("ProcesarRead - directoryId", osada_log);
	*directoryId = -1;

	// Genero mi buffer y lo inicializo
	char * buf = (char *)myMalloc_char(size, "ProcesarRead - buffer", osada_log);
	memset((void *)buf, 0, size);

	FindDirectoryByName(path , directoryId);

	sComenzarLecturaArchivo(*directoryId);
	int bytes_leidos_escritos = LectoEscrituraFromOffset((off_t)offset, (size_t)size, *directoryId, buf, LECTURA);
	sFinalizarLecturaArchivo(*directoryId);

	// Enviar el tamaño de los bytes a enviar
	enviarInt(fd, bytes_leidos_escritos);

	// Enviar los bytes...
	enviarBytes(fd, (void *)buf, bytes_leidos_escritos);

	myFree(result, "ProcesarRead - result", osada_log);
	myFree(path, "ProcesarRead - path", osada_log);
	myFree(directoryId, "ProcesarRead - directoryId", osada_log);
	myFree(buf, "ProcesarRead - buf", osada_log);

	// Envio el fin del read
	enviarInt(fd, FIN_READ);

}

void ProcesarMkDir(int fd)
{
	// Recibo el tamaño del path
	int * result = (int *)myMalloc_int("ProcesarMkDir - result", osada_log);
	int tamanioPath;
	tamanioPath = recibirInt(fd , result, osada_log);

	// Recibo el path
	char * path = myMalloc_char((tamanioPath+1), "ProcesarMkDir - path", osada_log);
	recibirMensaje(fd, path, tamanioPath, osada_log);

	// Recibir el codigo (entero)
	int codigoOperacion;
	codigoOperacion = recibirInt(fd , result, osada_log);

	int resultado = Crear(path, codigoOperacion);

	// Envio el resultado
	enviarInt(fd, resultado);

	// Envio el fin de la operacion
	enviarInt(fd, FIN_MKDIR);

	// Libero las variables
	myFree(result, "ProcesarMkDir - result", osada_log);
	myFree(path, "ProcesarMkDir - path", osada_log);

}

void ProcesarCreate(int fd)
{
	// Recibo el tamaño del path
	int * result = (int *)myMalloc_int("ProcesarCreate - result", osada_log);
	int tamanioPath;
	tamanioPath = recibirInt(fd , result, osada_log);

	// Recibo el path
	char * path = myMalloc_char((tamanioPath+1), "ProcesarCreate - path", osada_log);
	recibirMensaje(fd, path, tamanioPath, osada_log);

	// Recibir el codigo (entero)
	int codigoOperacion;
	codigoOperacion = recibirInt(fd , result, osada_log);

	int resultado = Crear(path, codigoOperacion);

	// Envio el resultado
	enviarInt(fd, resultado);

	// Envio el fin de la operacion
	enviarInt(fd, FIN_CREATE);

	// Libero las variables
	myFree(result, "ProcesarCreate - result", osada_log);
	myFree(path, "ProcesarCreate - path", osada_log);

}

void ProcesarWrite(int fd)
{
	// Recibo el tamaño del path
	int * result = (int *)myMalloc_int("ProcesarWrite - result", osada_log);
	int tamanioPath;
	tamanioPath = recibirInt(fd , result, osada_log);

	// Recibo el path
	char * path = myMalloc_char((tamanioPath+1), "ProcesarWrite - path", osada_log);
	recibirMensaje(fd, path, tamanioPath, osada_log);

	// Recibir el size (entero)
	int size;
	size = recibirInt(fd , result, osada_log);

	// Recibir el offset (entero)
	int offset;
	offset = recibirInt(fd , result, osada_log);

	// Genero mi buffer y lo inicializo
	char * buf = (char *)myMalloc_char(size, "ProcesarWrite - buffer", osada_log);
	memset((void *)buf, 0, size);

	// Llenar el buffer
	recibirBytesRaw(fd, (void *)buf, (int)size, osada_log);

	// Declaro mi variable para la entrada de la tabla de archivos
	int * directoryId = (int *)myMalloc_int("ProcesarWrite - directoryId", osada_log);
	*directoryId = -1;

	FindDirectoryByName(path , directoryId);

	sComenzarEscrituraArchivo(*directoryId);
	int bytes_leidos_escritos = LectoEscrituraFromOffset((off_t)offset, (size_t)size, *directoryId, buf, ESCRITURA);
	sFinalizarEscrituraArchivo(*directoryId);

	// Enviar la cantidad de bytes escritos
	enviarInt(fd, bytes_leidos_escritos);

	myFree(result, "ProcesarWrite - result", osada_log);
	myFree(path, "ProcesarWrite - path", osada_log);
	myFree(directoryId, "ProcesarWrite - directoryId", osada_log);
	myFree(buf, "ProcesarWrite - buf", osada_log);

	// Envio el fin del read
	enviarInt(fd, FIN_WRITE);

}

void ProcesarUnlink(int fd)
{
	// Recibo el tamaño del path
	int * result = (int *)myMalloc_int("ProcesarUnlink - result", osada_log);
	int tamanioPath;
	tamanioPath = recibirInt(fd , result, osada_log);

	// Recibo el path
	char * path = myMalloc_char((tamanioPath+1), "ProcesarUnlink - path", osada_log);
	recibirMensaje(fd, path, tamanioPath, osada_log);

	int * directoryId = myMalloc_int("ProcesarUnlink - directoryId", osada_log);
	int cantBloques = 0, status = 0;

	*directoryId = -1;

	FindDirectoryByName(path , directoryId);

	osada_file * indice_tabla_archivos = tabla_archivos;
	indice_tabla_archivos+=*directoryId;

	sComenzarEscrituraArchivo(*directoryId);
	cantBloques = TamanioEnBloques(indice_tabla_archivos->file_size);

	status = DeleteBlocks(cantBloques,*directoryId);

	if(!status){
		indice_tabla_archivos->state=0;
	}
	sFinalizarEscrituraArchivo(*directoryId);

	// Enviar Status al cliente...
	enviarInt(fd, status);

	// Liberar las variables
	myFree(result, "ProcesarUnlink - result", osada_log);
	myFree(path, "ProcesarUnlink - path", osada_log);
	myFree(directoryId, "ProcesarUnlink - directoryId", osada_log);

	// Enviar el fin de UNLINK
	enviarInt(fd, FIN_UNLINK);
}

void ProcesarTruncate(int fd)
{
	// Recibo el tamaño del path
	int * result = (int *)myMalloc_int("ProcesarTruncate - result", osada_log);
	int tamanioPath;
	tamanioPath = recibirInt(fd , result, osada_log);

	// Recibo el path
	char * path = myMalloc_char((tamanioPath+1), "ProcesarTruncate - path", osada_log);
	recibirMensaje(fd, path, tamanioPath, osada_log);

	// Recibo el length
	int * length = (int *)myMalloc_int("ProcesarTruncate - length", osada_log);
	int lengthRecibida;
	lengthRecibida = recibirInt(fd, length, osada_log);

	int * directoryId = myMalloc_int("ProcesarTruncate - directoryId", osada_log);
	int cantBloques = 0, status = 0;

	*directoryId = -1;

	FindDirectoryByName(path , directoryId);
	if (*directoryId < 0)
		*result = -ENOENT;

	sComenzarEscrituraArchivo(*directoryId);

	osada_file * indice_tabla_archivos = tabla_archivos;
	indice_tabla_archivos+=*directoryId;

	cantBloques = TamanioEnBloques(indice_tabla_archivos->file_size);

	DeleteBlocks(cantBloques,*directoryId);

	indice_tabla_archivos->first_block = SIN_BLOQUES_ASIGNADOS;
	indice_tabla_archivos->file_size = 0;

	sFinalizarEscrituraArchivo(*directoryId);

	myFree(result, "ProcesarTruncate - result", osada_log);
	myFree(path, "ProcesarTruncate - path", osada_log);
	myFree(directoryId, "ProcesarTruncate - directoryId", osada_log);

	enviarInt(fd, FIN_TRUNCATE);

}

void ProcesarRmDir(int fd)
{
	// Recibo el tamaño del path
	int * result = (int *)myMalloc_int("ProcesarRmDir - result", osada_log);
	int tamanioPath;
	tamanioPath = recibirInt(fd , result, osada_log);

	// Recibo el path
	char * path = myMalloc_char((tamanioPath+1), "ProcesarRmDir - path", osada_log);
	recibirMensaje(fd, path, tamanioPath, osada_log);

	int * directoryId = myMalloc_int("ProcesarRmDir - directoryId", osada_log);
	*directoryId = -1;

	osada_file *indice_tabla_archivos = tabla_archivos;
	osada_file *indice_tabla_archivos_busqueda = tabla_archivos;

	FindDirectoryByName(path, directoryId);

	indice_tabla_archivos+=*directoryId;

	int i = 0, status = 0;

 	while(i < OSADA_CANTIDAD_MAXIMA_ARCHIVOS)
 	{
 		if ((int)indice_tabla_archivos_busqueda->state != 0)
 		{
 			if (indice_tabla_archivos_busqueda->parent_directory == *directoryId)
 			{
 				status = -ENOTEMPTY;
 				break;
 			}
 		}
 		indice_tabla_archivos_busqueda++;
  		i++;
  	}

 	if(i == OSADA_CANTIDAD_MAXIMA_ARCHIVOS){
 		sComenzarEscrituraArchivo(*directoryId);
 		indice_tabla_archivos->state = 0;
 		sFinalizarEscrituraArchivo(*directoryId);
  		status = 0;
  	}


	myFree(result, "ProcesarRmDir - result", osada_log);
	myFree(path, "ProcesarRmDir - path", osada_log);
	myFree(directoryId, "ProcesarRmDir - directoryId", osada_log);

	enviarInt(fd, status);

	enviarInt(fd, RMDIR);

}

// todo: faltan semaforos
void ProcesarRename(int fd)
{
	// Recibo el tamaño del path viejo
	int * oldResult = (int *)myMalloc_int("ProcesarRename - newResult", osada_log);
	int tamanioOldPath;
	tamanioOldPath = recibirInt(fd , oldResult, osada_log);

	// Recibo el path viejo
	char * oldpath = myMalloc_char((tamanioOldPath+1), "ProcesarRename - path", osada_log);
	recibirMensaje(fd, oldpath, tamanioOldPath, osada_log);

	// Recibo el tamaño del path nuevo
	int * newResult = (int *)myMalloc_int("ProcesarRename - newResult", osada_log);
	int tamanioNewPath;
	tamanioNewPath = recibirInt(fd , newResult, osada_log);

	// Recibo el path nuevo
	char * newpath = myMalloc_char((tamanioNewPath+1), "ProcesarRename - path", osada_log);
	recibirMensaje(fd, newpath, tamanioNewPath, osada_log);

	int * directoryId = myMalloc_int("ProcesarRename - directoryId", osada_log);
	*directoryId = -1;

	int status = 0;

	osada_file *indice_tabla_archivos = tabla_archivos;


	FindDirectoryByName(oldpath,directoryId);
	indice_tabla_archivos+=*directoryId;


	if(TamanioNombreAdecuado(newpath))
	{

		if (!ExistsDirectoryByName(newpath))
		{
			sComenzarEscrituraArchivo(*directoryId);
			///
			char ** splitPath = SplitPath(newpath);

			int name = 0;
			while(splitPath[name] != NULL)
			{
				log_trace(osada_log, "RENAME nombre PARCIAL: %s", splitPath[name]);
				name++;
			}

			// TODO: ver si aca tengo que asignar memoria...
			char * nombre_archivo = myMalloc_char(OSADA_FILENAME_LENGTH, "Rename - nombre_archivo", osada_log);
			memset((void *)nombre_archivo, '\0', OSADA_FILENAME_LENGTH);
			memcpy((void *)nombre_archivo, (void *)splitPath[name-1], strlen(splitPath[name-1]));

			log_trace(osada_log, "RENAME nombre: %s", nombre_archivo);
			///
			int index = 0;

			while (index < 17)
			{
				if ((nombre_archivo[index]) != '\0')
					(indice_tabla_archivos)->fname[index] = (nombre_archivo[index]);
				else
					(indice_tabla_archivos)->fname[index] = '\0';
				index++;
			}
				status = 0;

			sFinalizarEscrituraArchivo(*directoryId);

		}
		else status = -ENOENT;

	}
	else status = -EFBIG;

	myFree(oldResult, "ProcesarRename - oldResult", osada_log);
	myFree(oldpath, "ProcesarRename - oldpath", osada_log);
	myFree(newResult, "ProcesarRename - newResult", osada_log);
	myFree(newpath, "ProcesarRename - newpath", osada_log);
	myFree(directoryId, "ProcesarRename - directoryId", osada_log);

	enviarInt(fd, status);
	enviarInt(fd, FIN_RENAME);

}

//todo: sincronizar
void ProcesarUtimens(int fd)
{
	// Recibo el tamaño del path
	int * result = (int *)myMalloc_int("ProcesarUtimens - result", osada_log);
	int tamanioPath;
	tamanioPath = recibirInt(fd , result, osada_log);

	// Recibo el path
	char * path = myMalloc_char((tamanioPath+1), "ProcesarUtimens - path", osada_log);
	recibirMensaje(fd, path, tamanioPath, osada_log);

	int * seconds = (int *)myMalloc_int("ProcesarUtimens - time",osada_log);
	int time;
	time = recibirInt(fd, seconds, osada_log);

	int * directoryId = myMalloc_int("ProcesarUtimens - directoryId", osada_log);
	*directoryId = -1;

	int status = 0;

	osada_file *indice_tabla_archivos = tabla_archivos;

	FindDirectoryByName(path, directoryId);
	if (*directoryId < 0){
		status = -ENOENT;
	}
	else
	{
		sComenzarEscrituraArchivo(*directoryId);
		indice_tabla_archivos+=*directoryId;
		indice_tabla_archivos->lastmod = time;
		status = FIN_UTIME;
		sFinalizarEscrituraArchivo(*directoryId);
	}


	myFree(result, "ProcesarUtimens - result", osada_log);
	myFree(path, "ProcesarUtimens - path", osada_log);
	myFree(seconds, "ProcesarUtimens - seconds", osada_log);
	myFree(directoryId, "ProcesarUtimens - directoryId", osada_log);

	enviarInt(fd, status);

}

void SetAttrByDirectoryId(int directoryId, int fd)
{
	sComenzarLecturaArchivo(directoryId);

	osada_file * indice_tabla_archivos = tabla_archivos;
	indice_tabla_archivos+=directoryId;

	bool esArchivo = strcmp(TipoDeArchivo(indice_tabla_archivos->state),"Ocupado") == 0;
	log_trace(osada_log, "%s esArchivo %d", indice_tabla_archivos->fname, esArchivo);

	if (esArchivo == 1)
	{
		enviarInt(fd, ST_MODE_CODE_ES_ARCHIVO);
	}
	else
	{
		enviarInt(fd, ST_MODE_CODE_ES_DIRECTORIO);
	}

	// Enviar el SIZE
	log_trace(osada_log, "Envio el tamaño del archivo: %d", indice_tabla_archivos->file_size);
	enviarInt(fd, (int)indice_tabla_archivos->file_size);

	enviarInt(fd, (int)indice_tabla_archivos->lastmod);

	sFinalizarLecturaArchivo(directoryId);

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

	sComenzarLecturaTablaArchivos();

	while(i < OSADA_CANTIDAD_MAXIMA_ARCHIVOS && !exists)
	{
		//sComenzarLecturaArchivo(i);

		if ((int)indice_tabla_archivos->state != 0)
		{
		//	int criterio_compare = indice_tabla_archivos->fname[TAMANIO_MAXIMO_NOMBRE_ARCHIVO-1] != '\0' ? TAMANIO_MAXIMO_NOMBRE_ARCHIVO : strlen(indice_tabla_archivos->fname);
		//	int lenght_compare = criterio_compare > strlen(path[0]) ? criterio_compare : strlen(path[0]);
		//	if (indice_tabla_archivos->parent_directory == parentId && memcmp(indice_tabla_archivos->fname,path[0],lenght_compare) == 0)
			if (indice_tabla_archivos->parent_directory == parentId && CompararNombres(indice_tabla_archivos->fname, path[0]))
			{
				if (path[1] != NULL)
				{
				//	sFinalizarLecturaArchivo(i);
					sFinalizarLecturaTablaArchivos();
					return FindDirectoryByNameAndParent(&path[1], (int)(indice_tabla_archivos - tabla_archivos), directoryId);
				}
				else
				{
				//	sFinalizarLecturaArchivo(i);
					exists = true;
					*directoryId = (int)(indice_tabla_archivos - tabla_archivos);
					log_trace(osada_log, "Encontrado!!!");
				}
			}
			else;
				//sFinalizarLecturaArchivo(i);
		}
		else;
			//sFinalizarLecturaArchivo(i);

		indice_tabla_archivos++;
		i++;
	}

	sFinalizarLecturaTablaArchivos();
	return exists;

}

void FindAllFilesByParentId(int * parentId, void *buf, fuse_fill_dir_t filler, int fd)
{

	osada_file * indice_tabla_archivos = tabla_archivos;
	int i = 0;

	sComenzarLecturaTablaArchivos();

		while(i < OSADA_CANTIDAD_MAXIMA_ARCHIVOS)
		{
			//sComenzarLecturaArchivo(i);

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

					log_trace(osada_log, "Liberando FindAllFilesByParentId - name: %s", name);
					myFree(name, "FindAllFilesByParentId - name: %s", osada_log);

				}
			}
			//sFinalizarLecturaArchivo(i);
			indice_tabla_archivos++;
			i++;
		}

		sFinalizarLecturaTablaArchivos();

		enviarInt(fd, FINALIZAR_READDIR);
}
int LectoEscrituraFromOffset(off_t offset_from, size_t bytes_to_rw, int directoryId, char * buf, int operacion)
{
	// Solo para la lectura
	char * aux_buf = myMalloc_char(bytes_to_rw, "LectoEscrituraFromOffset - auxbuf", osada_log);
	char * indice_aux_buf = aux_buf;

	// Ubico un puntero a la tabla de archivos para averiguar el primer bloque.
	osada_file * indice_tabla_archivos = tabla_archivos;
	indice_tabla_archivos += directoryId;

	int * indice_tabla_asignaciones = tabla_asignaciones;
	char * indice_tabla_datos = tabla_datos;

	int bloque_actual, bloque_anterior, bloque_inicial;

	// Ver como funciona la lectura de archivo que no tiene bloques asignados
	bloque_actual = AsignarBloqueActualLectoEscritura(indice_tabla_archivos, operacion, directoryId);

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

	bool obtuve_bloque_inicial = false;
	if (bloque_actual == indice_tabla_archivos->first_block)
	{
		bloque_inicial = bloque_actual;
		obtuve_bloque_inicial = true;
	}

	// Ubico el indice de los datos en el byte a comenzar a leer.
	indice_tabla_datos += bloque_actual * OSADA_BLOCK_SIZE;
	indice_tabla_datos += offset_from;

	int bytes_a_rw = 0;

	while (bytes_remaining > 0)
	{
		if (bytes_remaining > (OSADA_BLOCK_SIZE - offset_from))
			bytes_a_rw = OSADA_BLOCK_SIZE - offset_from;
		else
			bytes_a_rw = bytes_remaining;

		if (operacion == ESCRITURA)
		{
			//printf("%.*s", stringLength, pointerToString);
			log_trace(osada_log, "Escribi los siguientes bytes en el archivo %d del bloque %d: %.*s", directoryId, bloque_actual,bytes_a_rw, buf);
			memcpy((void *)indice_tabla_datos, buf, (bytes_a_rw));
			buf += bytes_a_rw;
		}
		else
		{
			log_trace(osada_log, "Lei los siguientes bytes en el archivo %d del bloque %d: %.*s", directoryId, bloque_actual, bytes_a_rw, indice_tabla_datos);
			memcpy((void *)indice_aux_buf, indice_tabla_datos, (bytes_a_rw));
			indice_aux_buf += bytes_a_rw;
		}
		indice_tabla_datos += bytes_a_rw;
		offset_from += bytes_a_rw;
		bytes_remaining -= bytes_a_rw;

		// Si llegue al final de un bloque, muevo el puntero de la tabla de asignaciones...
		if (offset_from == (OSADA_BLOCK_SIZE) && bytes_remaining != NO_HAY_MAS_BLOQUES_POR_RW)
		{
			bloque_actual = FinalDeBloqueLectoEscritura(indice_tabla_asignaciones, bloque_actual, operacion, directoryId);

			if (!obtuve_bloque_inicial)
			{
				bloque_inicial = bloque_actual;
				obtuve_bloque_inicial = true;
			}

			if (bloque_actual != NO_HAY_ESPACIO_BITMAP)
			{
				indice_tabla_datos = tabla_datos;
				indice_tabla_datos += bloque_actual * OSADA_BLOCK_SIZE;
				offset_from = 0;
			}
			else
			{
				//indice_tabla_archivos->file_size += bytes_to_rw; // Le sumo el tamaño para que dsp se borre todo.
				RollbackLectoEscrituraSinEspacio(indice_tabla_asignaciones, bloque_inicial);
				return -ENOSPC; // Corto la ejecucion aca porque me quede sin espacio.
			}
		}

	}

	if (operacion == ESCRITURA)
	{
		// Marco el fin de los bloques.
		*(indice_tabla_asignaciones + bloque_actual) = FIN_DE_DATOS_DE_ARCHIVO;
		indice_tabla_archivos->file_size += bytes_to_rw;

		// Ultima fecha de modificación
		time_t ahora = time(0);
		indice_tabla_archivos->lastmod = ahora;
	}
	else
	{
		memcpy(buf, ((char*)aux_buf), bytes_to_rw);
	}

	myFree(aux_buf, "LectoEscrituraFromOffset - auxbuf", osada_log);

	return bytes_to_rw;
}

int FinalDeBloqueLectoEscritura(int * indice_tabla_asignaciones, int bloque_actual, int operacion, int directoryId)
{
	int bloque_anterior;

	if (operacion == ESCRITURA)
	{
		bloque_anterior = bloque_actual;
		bloque_actual = BuscaPrimerEspacioDisponibleEnBitMap(directoryId);

		if (bloque_actual != NO_HAY_ESPACIO_BITMAP)
		{
			SeteaBitEnBitMap(bloque_actual);
			*(indice_tabla_asignaciones + bloque_anterior) = bloque_actual;
		}
		else
		{
			// No tengo mas espacio. Marco que finaliza lo escrito para saber hasta cuando borrar...
			*(indice_tabla_asignaciones + bloque_anterior) = NO_HAY_ESPACIO_BITMAP;
		}
	}
	else
	{
		bloque_actual = *(indice_tabla_asignaciones + bloque_actual);
	}

	return bloque_actual; // Es un nuevo bloque Actual o -1 si no hay mas bloques

}

int AsignarBloqueActualLectoEscritura(osada_file * indice_tabla_archivos, int operacion, int directoryId)
{
	int aux_bloque_actual;

	if (indice_tabla_archivos->first_block == SIN_BLOQUES_ASIGNADOS)
	{
		// No tiene bloque inicial, asignarlo. // Solo para escritura
		if (operacion == ESCRITURA)
		{
			aux_bloque_actual = BuscaPrimerEspacioDisponibleEnBitMap(directoryId);

			if (aux_bloque_actual != NO_HAY_ESPACIO_BITMAP)
			{
				indice_tabla_archivos->first_block = aux_bloque_actual;
				SeteaBitEnBitMap(aux_bloque_actual);
			}
		}
	}
	else
	{
		aux_bloque_actual = indice_tabla_archivos->first_block;
	}

	return aux_bloque_actual;
}

int BuscaPrimerEspacioDisponibleEnBitMap(int directoryId)
{
	sComenzarEscrituraBitMap();

	//int index = 0;
	int centinela = 0;
	bool hayEspacio = false;
	t_bitarray * bm = bitmap;

	while(!hayEspacio && centinela < header->data_blocks)
	{
		if (ultimo_bloque_disponible_encontrado == header->data_blocks)
			ultimo_bloque_disponible_encontrado = 0;

		int bit_value = (int)bitarray_test_bit(bm, offset_bitmap_datos + ultimo_bloque_disponible_encontrado);

		if (bit_value == 0)
		{
			hayEspacio = true;
			SeteaBitEnBitMap(ultimo_bloque_disponible_encontrado);
			log_trace(osada_log, "Reserve el bloque %d al archivo %d", ultimo_bloque_disponible_encontrado, directoryId);
		}
		else
			ultimo_bloque_disponible_encontrado++;

		centinela++;

	}

	int ultimo_bloque_disponible_encontrado_local = ultimo_bloque_disponible_encontrado;

	sFinalizarEscrituraBitMap();

	return hayEspacio ? ultimo_bloque_disponible_encontrado_local : NO_HAY_ESPACIO_BITMAP;
}

void SeteaBitEnBitMap(int offset)
{
	t_bitarray * bm = bitmap;
	bitarray_set_bit(bm, offset_bitmap_datos + offset);
}

void LimpiaBitEnBitMap(int offset)
{
	t_bitarray * bm = bitmap;
	bitarray_clean_bit(bm, offset_bitmap_datos + offset);
}

int Crear(char * path, int state)
{
	// Chequear espacio disponible
	int returnValue = -1;

		// Chequear longitud del nombre
		if (TamanioNombreAdecuado(path))
		{
			// Chequear que no exista otro igual
			if (!ExistsDirectoryByName(path))
			{
				// Ubicar el ID del directorio padre
				int * parentDirectoryId = myMalloc_int("Crear - parentDirectoryId", osada_log);
				*parentDirectoryId = -1;
				if (FindParentDirectoryByName(path, parentDirectoryId))
				{
					// Chequear espacio disponible
					int tablaArchivosId = BuscaPrimerEspacioDisponibleEnTablaArchivos();

					if (tablaArchivosId > NO_HAY_ESPACIO_TABLA_ARCHIVOS)
					{
						// Generar la carpeta
						CrearArchivoDirectorio(path, tablaArchivosId, *parentDirectoryId, state);
						returnValue = 0;
					}
					else
						returnValue = -ENOSPC;
				}
				else
					returnValue = -ENOENT;

				myFree(parentDirectoryId, "Crear - parentDirectoryId", osada_log);
			}
			else
				returnValue = -ENOENT;
		}
		else
			returnValue = -ENAMETOOLONG
;

	return returnValue;

}

int BuscaPrimerEspacioDisponibleEnTablaArchivos()
{
	sComenzarEscrituraTablaArchivos();

	osada_file * indice_tabla_archivos = tabla_archivos;
	int index = 0;
	bool hayEspacio = false;

	while(!hayEspacio && index < OSADA_CANTIDAD_MAXIMA_ARCHIVOS)
	{
		if ((indice_tabla_archivos+index)->state == 0)
		{
			hayEspacio = true;
			(indice_tabla_archivos+index)->state = 1; // Esto lo pongo para reservar el lugar, luego en la creacion
										// se corrige dependiendo si es ARCHIVO o DIRECTORIO.
		}
		else
			index++;
	}

	sFinalizarEscrituraTablaArchivos();

	return hayEspacio ? index : NO_HAY_ESPACIO_TABLA_ARCHIVOS;

}

bool TamanioNombreAdecuado(char * path)
{
	char ** splitPath = SplitPath(path);

	bool returnValue = (strlen(splitPath[LongitudArray(splitPath)-1]) <= TAMANIO_MAXIMO_NOMBRE_ARCHIVO);

	log_trace(osada_log, "Voy a liberar el path: %s", path);
	myFreeSplitPath(splitPath, "FindDirectoryByName - arr", osada_log);

	return returnValue;
}

bool ExistsDirectoryByName(char * path)
{
	char ** arr = SplitPath(path);
	int * directoryId = myMalloc_int("ExistsDirectoryByName - directoryId", osada_log);
	*directoryId = -1;
	bool returnValue = FindDirectoryByNameAndParent(arr, 0xffff, directoryId);

	// Libero las variables
	log_trace(osada_log, "Voy a liberar el path: %s", path);
	myFreeSplitPath(arr, "ExistsDirectoryByName - arr", osada_log);
	myFree(directoryId, "ExistsDirectoryByName - directoryId", osada_log);

	return returnValue;
}

bool FindParentDirectoryByName(char * path, int * directoryId)
{
	char ** arr = SplitPath(path);
	int lArray = LongitudArray(arr);
	bool returnValue;

	if (lArray == 1)
		returnValue = 0xffff; // Es un directorio en el directorio raiz.
	else
	{
		arr[lArray - 1] = NULL;
		log_trace(osada_log, arr[0]);
		returnValue = FindDirectoryByNameAndParent(arr, 0xffff, directoryId);
	}

	// Libero las variables
	log_trace(osada_log, "Voy a liberar el path: %s", path);
	myFreeSplitPath(arr, "FindParentDirectoryByName - arr", osada_log);

	return returnValue;
}

int LongitudArray(char ** arr)
{
	int index = 0;

	while(arr[index] != NULL)
		index++;

	return index;
}

void CrearArchivoDirectorio(char * path, int tablaArchivosId, int parentDirectoryId, int state)
{
	sComenzarEscrituraArchivo(tablaArchivosId);

	log_trace(osada_log, "Comienzo a Crear el directorio...%s", path);
	osada_file * indice_tabla_archivos = tabla_archivos;

	char ** splitPath = SplitPath(path);

	int name = 0;
	while(splitPath[name] != NULL)
	{
		log_trace(osada_log, "nombre PARCIAL: %s", splitPath[name]);
		name++;
	}

	// TODO: ver si aca tengo que asignar memoria...
	char * nombre_archivo = myMalloc_char(OSADA_FILENAME_LENGTH, "CrearArchivoDirectorio - nombre_archivo", osada_log);
	memset((void *)nombre_archivo, '\0', OSADA_FILENAME_LENGTH);
	memcpy((void *)nombre_archivo, (void *)splitPath[name-1], strlen(splitPath[name-1]));

	log_trace(osada_log, "nombre: %s", nombre_archivo);

	int index = 0;

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
	(indice_tabla_archivos+tablaArchivosId)->lastmod = time(0);
//
//	log_trace(osada_log, "Fin creacion del directorio...%s - con padre id: %04x", path, (indice_tabla_archivos+tablaArchivosId)->parent_directory);
//	if (parentDirectoryId == -1)
//	log_trace(osada_log, "Padre Original: %04x - Padre Guardado: %04x", 0xffff, (indice_tabla_archivos+tablaArchivosId)->parent_directory);
//	else
//		log_trace(osada_log, "Padre Original: %04x - Padre Guardado: %04x", parentDirectoryId, (indice_tabla_archivos+tablaArchivosId)->parent_directory);
//
//	log_trace(osada_log, "File Size: %d", (indice_tabla_archivos+tablaArchivosId)->file_size);

	log_trace(osada_log, "Voy a liberar el path: %s", path);
	myFreeSplitPath(splitPath, "CrearArchivoDirectorio - splitPath", osada_log);
	myFree(nombre_archivo, "CrearArchivoDirectorio - nombre_archivo", osada_log);

	sFinalizarEscrituraArchivo(tablaArchivosId);
}

int DeleteBlocks(size_t blocks_to_delete, int directoryId)
{
	sComenzarEscrituraBitMap();

	bool algo;
	int i = 0;
	int offset_tabla_datos = 1 + header->bitmap_blocks + OSADA_TABLA_ARCHIVOS_SIZE + tamanio_tabla_asignaciones_bloques;

	t_bitarray *bitmap_aux = bitmap;

	osada_file * indice_tabla_archivos = tabla_archivos;
	indice_tabla_archivos+=directoryId;

	int * indice_tabla_asignaciones = tabla_asignaciones;

	int bloque_actual = indice_tabla_archivos->first_block;

	while(i<blocks_to_delete && i<header->data_blocks)
	//while(*(indice_tabla_asignaciones + bloque_actual) != FIN_DE_DATOS_DE_ARCHIVO)
	{
		algo = bitarray_test_bit(bitmap_aux,offset_tabla_datos+bloque_actual);

		bitarray_clean_bit(bitmap_aux,offset_tabla_datos+bloque_actual);
		i++;
		bloque_actual = *(indice_tabla_asignaciones + bloque_actual);
	}

	sFinalizarEscrituraBitMap();

	return 0;
}

void RollbackLectoEscrituraSinEspacio(int * indice_tabla_asignaciones, int bloque_inicial)
{
	int bloque_actual = bloque_inicial;

	sComenzarEscrituraBitMap();

	while(*(indice_tabla_asignaciones + bloque_actual) != NO_HAY_ESPACIO_BITMAP)
	{
		LimpiaBitEnBitMap(bloque_actual);
		bloque_actual = *(indice_tabla_asignaciones + bloque_actual);
	}

	LimpiaBitEnBitMap(bloque_actual); // Limpio el ultimo...

	sFinalizarEscrituraBitMap();

	*(indice_tabla_asignaciones + bloque_inicial) = NO_HAY_ESPACIO_BITMAP;
}

bool CompararNombres(unsigned char * nom1, char * nom2)
{
	char * nombre_archivo1 = myMalloc_char(OSADA_FILENAME_LENGTH, "CompararNombres - nombre_archivo1", osada_log);
	char * nombre_archivo2 = myMalloc_char(OSADA_FILENAME_LENGTH, "CompararNombres - nombre_archivo2", osada_log);
	memset((void *)nombre_archivo1, '\0', OSADA_FILENAME_LENGTH);
	memset((void *)nombre_archivo2, '\0', OSADA_FILENAME_LENGTH);

	int index = 0;
	bool done1 = false, done2 = false;

	while (index < OSADA_FILENAME_LENGTH)
	{
		if ((nom1[index]) != '\0' && !done1)
			nombre_archivo1[index] = nom1[index];
		else
			done1 = true;

		if ((nom2[index]) != '\0' && !done2)
			nombre_archivo2[index] = nom2[index];
		else
			done2 = true;

		index++;
	}

	bool returnValue =  strcmp(nombre_archivo1, nombre_archivo2) == 0;

	myFree(nombre_archivo1, "CompararNombres - nombre_archivo1", osada_log);
	myFree(nombre_archivo2, "CompararNombres - nombre_archivo2", osada_log);

	return returnValue;

}

// Sincronizacion
void InicializarSemaforos()
{
	int index = 0;
	// Tabla de archivos
	myInitMutex(&mutex_escritura_tabla_archivos, "mutex_escritura_tabla_archivos", osada_log);
	myInitMutex(&mutex_cuenta_lectores_tabla_archivos, "mutex_cuenta_lectores_tabla_archivos", osada_log);
	int cuenta_lectores_tabla_archivos = 0;

	// Bitmap
	myInitMutex(&mutex_escritura_bitmap, "mutex_escritura_bitmap", osada_log);
	myInitMutex(&mutex_cuenta_lectores_bitmap, "mutex_cuenta_lectores_bitmap", osada_log);
	cuenta_lectores_bitmap = 0;
	ultimo_bloque_disponible_encontrado = 0;


	//Archivos
	mutex_escritura_archivos = (pthread_mutex_t *)myMalloc_mutex_t(OSADA_CANTIDAD_MAXIMA_ARCHIVOS, "Puntero - mutex_escritura_archivos", osada_log);
	mutex_cuenta_lectores_archivos = (pthread_mutex_t *)myMalloc_mutex_t(OSADA_CANTIDAD_MAXIMA_ARCHIVOS, "Puntero - mutex_cuenta_lectores_archivos", osada_log);
	cuenta_lectores_archivos = (int *)myMalloc_int_array(OSADA_CANTIDAD_MAXIMA_ARCHIVOS, "Int Array - cuenta_lectores_archivos", osada_log);

	for (index = 0 ; index < OSADA_CANTIDAD_MAXIMA_ARCHIVOS ; index++)
	{
		myInitMutex(&(mutex_escritura_archivos[index]),"mutex_escritura_archivos[index]", osada_log);
		myInitMutex(&(mutex_escritura_archivos[index]),"mutex_escritura_archivos[index]", osada_log);
		cuenta_lectores_archivos[index] = 0;
	}

}

void sComenzarLecturaTablaArchivos()
{
	log_trace(osada_log, "**** SINCRO: COMENZAR LECTURA TABLA ARCHIVOS");
	myMutexLock(&mutex_cuenta_lectores_tabla_archivos, "mutex_cuenta_lectores_tabla_archivos", osada_log);
	cuenta_lectores_tabla_archivos++;
	log_trace(osada_log, "Cantidad Lectores: %d", cuenta_lectores_tabla_archivos);
	if (cuenta_lectores_tabla_archivos == 1)
		myMutexLock(&mutex_escritura_tabla_archivos, "mutex_escritura_tabla_archivos", osada_log);

	myMutexUnlock(&mutex_cuenta_lectores_tabla_archivos, "mutex_cuenta_lectores_tabla_archivos", osada_log);
}

void sFinalizarLecturaTablaArchivos()
{
	log_trace(osada_log, "**** SINCRO: FINALIZAR LECTURA TABLA ARCHIVOS");
	myMutexLock(&mutex_cuenta_lectores_tabla_archivos, "mutex_cuenta_lectores_tabla_archivos", osada_log);
	cuenta_lectores_tabla_archivos--;
	log_trace(osada_log, "Cantidad Lectores: %d", cuenta_lectores_tabla_archivos);
	if (cuenta_lectores_tabla_archivos == 0)
		myMutexUnlock(&mutex_escritura_tabla_archivos, "mutex_escritura_tabla_archivos", osada_log);

	myMutexUnlock(&mutex_cuenta_lectores_tabla_archivos, "mutex_cuenta_lectores_tabla_archivos", osada_log);
}

void sComenzarEscrituraTablaArchivos()
{
	log_info(osada_log, "**** SINCRO: COMENZAR ESCRITURA TABLA ARCHIVOS");
	myMutexLock(&mutex_escritura_tabla_archivos, "mutex_escritura_tabla_archivos", osada_log);

}

void sFinalizarEscrituraTablaArchivos()
{
	log_info(osada_log, "**** SINCRO: FINALIZAR ESCRITURA TABLA ARCHIVOS");
	myMutexUnlock(&mutex_escritura_tabla_archivos, "mutex_escritura_tabla_archivos", osada_log);
}


void sComenzarLecturaBitmap()
{
	log_trace(osada_log, "**** SINCRO: COMENZAR LECTURA BITMAP");
	myMutexLock(&mutex_cuenta_lectores_bitmap, "mutex_cuenta_lectores_bitmap", osada_log);
	cuenta_lectores_bitmap++;
	log_trace(osada_log, "Cantidad Lectores BitMap: %d", cuenta_lectores_bitmap);
	if (cuenta_lectores_bitmap == 1)
		myMutexLock(&mutex_escritura_bitmap, "mutex_escritura_bitmap", osada_log);

	myMutexUnlock(&mutex_cuenta_lectores_bitmap, "mutex_cuenta_lectores_bitmap", osada_log);

}

void sFinalizarLecturaBitMap()
{
	log_trace(osada_log, "**** SINCRO: FINALIZAR LECTURA BITMAP");
	myMutexLock(&mutex_cuenta_lectores_bitmap, "mutex_cuenta_lectores_bitmap", osada_log);
	cuenta_lectores_bitmap--;
	log_trace(osada_log, "Cantidad Lectores: %d", mutex_cuenta_lectores_bitmap);
	if (cuenta_lectores_bitmap == 0)
		myMutexUnlock(&mutex_escritura_bitmap, "mutex_escritura_bitmap", osada_log);

	myMutexUnlock(&mutex_cuenta_lectores_bitmap, "mutex_cuenta_lectores_bitmap", osada_log);
}

void sComenzarEscrituraBitMap()
{
	log_info(osada_log, "**** SINCRO: COMENZAR ESCRITURA BITMAP");
	myMutexLock(&mutex_escritura_bitmap, "mutex_escritura_bitmap", osada_log);

}

void sFinalizarEscrituraBitMap()
{
	log_info(osada_log, "**** SINCRO: FINALIZAR ESCRITURA BITMAP");
	myMutexUnlock(&mutex_escritura_bitmap, "mutex_escritura_bitmap", osada_log);
}

void sComenzarLecturaArchivo(int idArchivo)
{
	log_trace(osada_log, "**** SINCRO: COMENZAR LECTURA ARCHIVO: %d", idArchivo);
	myMutexLock(&mutex_cuenta_lectores_archivos[idArchivo], "mutex_cuenta_lectores_archivos", osada_log);
	cuenta_lectores_archivos[idArchivo] += 1;
	log_trace(osada_log, "Cantidad Lectores Archivo %d: %d", idArchivo, (cuenta_lectores_archivos[idArchivo]));
	if (cuenta_lectores_archivos[idArchivo] == 1)
		myMutexLock(&mutex_escritura_archivos[idArchivo], "mutex_escritura_archivos[idArchivo]", osada_log);

	myMutexUnlock(&mutex_cuenta_lectores_archivos[idArchivo], "mutex_cuenta_lectores_archivos[idArchivo]", osada_log);

}

void sFinalizarLecturaArchivo(int idArchivo)
{
	log_trace(osada_log, "**** SINCRO: FINALIZAR LECTURA ARCHIVO: %d", idArchivo);
	myMutexLock(&mutex_cuenta_lectores_archivos[idArchivo], "mutex_cuenta_lectores_archivos", osada_log);
	cuenta_lectores_archivos[idArchivo] -= 1;
	log_trace(osada_log, "Cantidad Lectores Archivo %d: %d", idArchivo, cuenta_lectores_archivos[idArchivo]);
	if (cuenta_lectores_archivos[idArchivo] == 0)
		myMutexUnlock(&mutex_escritura_archivos[idArchivo], "mutex_escritura_archivos[idArchivo]", osada_log);

	myMutexUnlock(&mutex_cuenta_lectores_archivos[idArchivo], "mutex_cuenta_lectores_archivos[idArchivo]", osada_log);


}

void sComenzarEscrituraArchivo(int idArchivo)
{
	log_info(osada_log, "**** SINCRO: COMENZAR ESCRITURA ARCHIVO: %d ", idArchivo);
	myMutexLock(&mutex_escritura_archivos[idArchivo], "mutex_escritura_archivos[idArchivo]", osada_log);
}

void sFinalizarEscrituraArchivo(int idArchivo)
{
	log_info(osada_log, "**** SINCRO: FINALIZAR ESCRITURA ARCHIVO: %d", idArchivo);
	myMutexUnlock(&mutex_escritura_archivos[idArchivo], "mutex_escritura_archivos[idArchivo]", osada_log);
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
//		if ((int)indice_tabla_archivos->state != 0)
//		{
			log_info(osada_log, " ---- Archivo %d ---- ", j);
			log_info(osada_log, "Estado: %s", TipoDeArchivo((int)indice_tabla_archivos->state));
			log_info(osada_log, "Nombre: %s", indice_tabla_archivos->fname);
			log_info(osada_log, "Tamanio: %d bytes - %d bloques", indice_tabla_archivos->file_size, TamanioEnBloques(indice_tabla_archivos->file_size));
			log_info(osada_log, "Directorio Padre: %04x hexa - %d decimal", indice_tabla_archivos->parent_directory, indice_tabla_archivos->parent_directory);
			log_info(osada_log, "Primer Bloque: %04x hexa - %d decimal", indice_tabla_archivos->first_block, indice_tabla_archivos->first_block);
			ImprimirBloquesDeTablaAsignacion(tabla_asignaciones, indice_tabla_archivos->file_size, indice_tabla_archivos->first_block);
			log_info(osada_log, "Fecha Ultima Modificacion: %d\n", indice_tabla_archivos->lastmod);
		//	GenerarArchivo(indice_datos, indice_tabla_archivos->file_size, indice_tabla_archivos->first_block, &(indice_tabla_archivos->fname));
//		}
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

