/*
 ============================================================================
 Name        : PokedexCliente.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>

#include "PokedexCliente.h"




int main(int argc, char *argv[]) {
	pokedex_cliente_log = CreacionLogWithLevel("pokedex-cliente-log", "pokedex-cliente", "TRACE");

	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	// Esta funcion de FUSE lee los parametros recibidos y los intepreta
	if (fuse_opt_parse(&args, NULL, fuse_options, NULL) == -1)
	{
		/** error parsing options */
		perror("Invalid arguments!");
		return EXIT_FAILURE;
	}

	/************ Creacion del Config ***********************/
	pokedex_cliente_config = creacion_config();

	if (pokedex_cliente_config->properties->elements_amount < 1)
	{
		perror("Error al abrir el archivo de configuracion del Pokedex Cliente");
		exit(0);
	}

//	char * puntoMontaje = malloc(sizeof(char));
//	puntoMontaje = config_get_string_value(pokedex_cliente_config , PUERTO_POKEDEX_SERVIDOR);

	//puertoServer =malloc(sizeof(char));
	puertoServer = config_get_string_value(pokedex_cliente_config , PUERTO_POKEDEX_SERVIDOR);

	//ipServer = malloc(sizeof(char));
	ipServer = config_get_string_value(pokedex_cliente_config , IP_POKEDEX_SERVIDOR);

	ltn_pokedex_cliente = createClientSocket(ipServer, puertoServer);
	clientSocket = doConnect(ltn_pokedex_cliente);

	// Esta es la funcion principal de FUSE, es la que se encarga
	// de realizar el montaje, comuniscarse con el kernel, delegar todo
	// en varios threads
	//char * mount_point = argv[2];
	system("umount /home/utnso/osada_mount");

	return fuse_main(args.argc, args.argv, &osada_oper, NULL);

}



// FUSE OPERATIONS
static int osada_getattr(const char *path, struct stat *stbuf) {
	log_trace(pokedex_cliente_log, "\t***** COMIENZO UN GETATTR. FD %d. *****", clientSocket);

	int res = 0;
	memset(stbuf, 0, sizeof(struct stat));

	// TODO: Quitar esto para prd...
	if (strstr(path, ".Trash") != NULL ||
			strstr(path, ".Trash-1000") != NULL ||
				strstr(path, "xdg-volume-info") != NULL	||
					strstr(path, "autorun.inf") != NULL 	)
		return -ENOENT;

	if (strcmp(path, "/" )== 0)
	{
		log_trace(pokedex_cliente_log, "Entro a '/'");
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	}
	else
	{
		log_trace(pokedex_cliente_log, "Entro a GETATTR -> 'ELSE'. Path: %s", path);

		// Envio la señal de GETATTR
		enviarInt(clientSocket, GETATTR);
		// Envio el tamaño del path
		enviarInt(clientSocket, strlen(path));
		// Envio el path
		enviarMensaje(clientSocket, path);

		// Recibir codigo de existencia de GETATTR
		int * result = myMalloc_int("osada_getattr - result", pokedex_cliente_log);
		int codigoExistencia;
		codigoExistencia = recibirInt(clientSocket , result, pokedex_cliente_log);
		// Si es aceptado, recibir las variables.

		if (codigoExistencia == ARCHIVO_NO_ENCONTRADO)
		{
			res = -ENOENT;
		}
		else
		{
			int valores;
			valores = recibirInt(clientSocket, result, pokedex_cliente_log);

			switch(valores)
			{
			case ST_MODE_CODE_ES_ARCHIVO:
				stbuf->st_mode = S_IFREG | 0444;
				break;
			case ST_MODE_CODE_ES_DIRECTORIO:
				stbuf->st_mode = S_IFDIR | 0755;
				break;
			}

			stbuf->st_nlink = 1;

			valores = recibirInt(clientSocket, result, pokedex_cliente_log);
			stbuf->st_size = valores;
		}
		myFree(result, "osada_getattr - result", pokedex_cliente_log);
	}




	// Espera la confirmacion del servidor para continuar...
	if (strcmp("/", path) != 0)
	{
		int * resultFin = myMalloc_int("osada_getattr resultFin", pokedex_cliente_log);
		int finFlag = recibirInt(clientSocket, resultFin, pokedex_cliente_log);
		if (finFlag == FIN_GETATTR)
		{
			log_trace(pokedex_cliente_log, "Finalizo el GETATTR - Cliente %d - Path %s", clientSocket, path);
		}
		else
		{
			log_error(pokedex_cliente_log, "ERROR EN EL GETATTR - Cliente %d - Path %s", clientSocket, path);
		}

		myFree(resultFin, "osada_getattr resultFin", pokedex_cliente_log);
	}

	log_trace(pokedex_cliente_log, "\t***** FINALIZO GETATTR. FD %d. *****", clientSocket);

	return res;

}

static int osada_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
	// Enviar la señal READDIR

	log_trace(pokedex_cliente_log, "\t***** Comienzo un READDIR *****");

	int res = enviarInt(clientSocket, READDIR);

	// Envio el tamanio del path
	enviarInt(clientSocket, strlen(path));

	// Envio el path
	enviarMensaje(clientSocket, path);

	int * result;
	int tamanioName = 0, flagFin = 0;
	char * name;
	int flag_finalizacion = CONTINUAR_READDIR;
	while(flag_finalizacion == CONTINUAR_READDIR)
	{
		result = myMalloc_int("osada_readdir - result", pokedex_cliente_log);

		flagFin = recibirInt(clientSocket, result, pokedex_cliente_log);

		if (flagFin == CONTINUAR_READDIR)
		{
			tamanioName = recibirInt(clientSocket, result, pokedex_cliente_log);

			name = myMalloc_char(tamanioName+1, "osada_readdir - name", pokedex_cliente_log);
			//recibirMensaje(clientSocket, name, tamanioName, pokedex_cliente_log);
			recibirBytes(clientSocket, (void *)name, 18, pokedex_cliente_log);

			filler(buf, name, NULL, 0);

			myFree(name, "osada_readdir - name", pokedex_cliente_log);
		}
		else
			flag_finalizacion = FINALIZAR_READDIR;

		myFree(result, "osada_readdir - result", pokedex_cliente_log);
	}

	// "." y ".." son entradas validas, la primera es una referencia al directorio donde estamos parados
	// y la segunda indica el directorio padre
	// TODO: Hay que dejar estos???
	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	//filler(buf, DEFAULT_FILE_NAME, NULL, 0);
	// Espera la confirmacion del servidor para continuar...
	int * resultFin = myMalloc_int("osada_getattr finFlag", pokedex_cliente_log);
	int finFlag = recibirInt(clientSocket, resultFin, pokedex_cliente_log);

	if (finFlag == FIN_READDIR)
	{
		log_trace(pokedex_cliente_log, "Finalizo el READDIR - Cliente %d - Path %s", clientSocket, path);
	}
	else
	{
		log_error(pokedex_cliente_log, "ERROR EN EL READDIR - Cliente %d - Path %s", clientSocket, path);
	}

	myFree(resultFin, "osada_readdir resultFin", pokedex_cliente_log);


	log_trace(pokedex_cliente_log, "\t***** FINALIZO un READDIR *****");
	//filler(buf, "archivo.txt", NULL, 0);


	return 0;
}

static int osada_read(const char *path, char *buf, size_t size, off_t offset,struct fuse_file_info *fi)
{

	log_trace(pokedex_cliente_log, "\t***** Comienzo un READ. Archivo: %s *****", path);

	// Envio el pedido de READ
	enviarInt(clientSocket, READ);

	// Envio el tamanio del path
	enviarInt(clientSocket, strlen(path));

	// Envio el path
	enviarMensaje(clientSocket, path);

	// Envio el size
	enviarInt(clientSocket, (int)size);

	// Envio el offset
	enviarInt(clientSocket, (int)offset);

	// Recibir la cantidad de bytes
	int * result = (int *)myMalloc_int("osada_read - result", pokedex_cliente_log);
	int cantidadBytes;
	cantidadBytes = recibirInt(clientSocket , result, pokedex_cliente_log);

	// Llenar el buffer
	recibirBytesRaw(clientSocket, (void *)buf, cantidadBytes, pokedex_cliente_log);

	myFree(result, "osada_read - Cliente - result", pokedex_cliente_log);

	// Espera la confirmacion del servidor para continuar...
	int * resultFin = myMalloc_int("osada_read resultFin", pokedex_cliente_log);
	int finFlag = recibirInt(clientSocket, resultFin, pokedex_cliente_log);
	if (finFlag == FIN_READ)
	{
		log_trace(pokedex_cliente_log, "Finalizo el READ - Cliente %d - Path %s", clientSocket, path);
	}
	else
	{
		log_error(pokedex_cliente_log, "ERROR EN EL READ - Cliente %d - Path %s", clientSocket, path);
	}

	myFree(resultFin, "osada_read resultFin", pokedex_cliente_log);

	log_trace(pokedex_cliente_log, "\t***** Finalizo un read un READ. Archivo: %s. Bytes: %d *****", path, cantidadBytes);

	return cantidadBytes;
}

static int osada_write (const char * path, const char * buf, size_t size, off_t offset, struct fuse_file_info * fi)
{
	log_trace(pokedex_cliente_log, "\t***** Comienzo un WRITE. Archivo: %s *****", path);

	// Envio el pedido de WRITE
	enviarInt(clientSocket, WRITE);

	// Envio el tamanio del path
	enviarInt(clientSocket, strlen(path));

	// Envio el path
	enviarMensaje(clientSocket, path);

	// Envio el size
	enviarInt(clientSocket, (int)size);

	// Envio el offset
	enviarInt(clientSocket, (int)offset);

	// Envio el buffer
	enviarBytes(clientSocket, (void *)buf, (int)size);

	// Recibir la cantidad de bytes
	int * result = (int *)myMalloc_int("osada_write - result", pokedex_cliente_log);
	int cantidadBytes;
	cantidadBytes = recibirInt(clientSocket , result, pokedex_cliente_log);

	myFree(result, "osada_write - Cliente - result", pokedex_cliente_log);

	// Espera la confirmacion del servidor para continuar...
	int * resultFin = myMalloc_int("osada_write resultFin", pokedex_cliente_log);
	int finFlag = recibirInt(clientSocket, resultFin, pokedex_cliente_log);
	if (finFlag == FIN_WRITE)
	{
		log_trace(pokedex_cliente_log, "Finalizo el WRITE - Cliente %d - Path %s", clientSocket, path);
	}
	else
	{
		log_error(pokedex_cliente_log, "ERROR EN EL WRITE - Cliente %d - Path %s", clientSocket, path);
	}

	myFree(resultFin, "osada_write resultFin", pokedex_cliente_log);

	log_trace(pokedex_cliente_log, "\t***** Finalizo un read un WRITE. Archivo: %s. Bytes: %d *****", path, cantidadBytes);

	return cantidadBytes;
}

static int osada_create (const char * path, mode_t mode, struct fuse_file_info * fi)
{
	log_trace(pokedex_cliente_log, "\t***** Comienzo un CREATE. Path: %s *****", path);

	int resultado = CrearArchivoDirectorio(path, CREATE, CREATE_FILE);

	// Espera la confirmacion del servidor para continuar...
	int * resultFin = myMalloc_int("osada_create resultFin", pokedex_cliente_log);
	int finFlag = recibirInt(clientSocket, resultFin, pokedex_cliente_log);
	if (finFlag == FIN_CREATE)
	{
		log_trace(pokedex_cliente_log, "Finalizo el CREATE - Cliente %d - Path %s", clientSocket, path);
	}
	else
	{
		log_error(pokedex_cliente_log, "ERROR EN EL CREATE - Cliente %d - Path %s", clientSocket, path);
	}

	myFree(resultFin, "osada_create resultFin", pokedex_cliente_log);

	log_trace(pokedex_cliente_log, "\tFINALIZO un CREATE. Path: %s *****", path);

	return resultado;
}

static int osada_mkdir (const char * path, mode_t mode)
{
	log_trace(pokedex_cliente_log, "\t***** Comienzo un MKDIR. Path: %s *****", path);

	int resultado = CrearArchivoDirectorio(path, MKDIR, MKDIR_DIRECTORIO);

	// Espera la confirmacion del servidor para continuar...
	int * resultFin = myMalloc_int("osada_mkdir resultFin", pokedex_cliente_log);
	int finFlag = recibirInt(clientSocket, resultFin, pokedex_cliente_log);
	if (finFlag == FIN_MKDIR)
	{
		log_trace(pokedex_cliente_log, "Finalizo el MKDIR - Cliente %d - Path %s", clientSocket, path);
	}
	else
	{
		log_error(pokedex_cliente_log, "ERROR EN EL MKDIR - Cliente %d - Path %s", clientSocket, path);
	}

	myFree(resultFin, "osada_mkdir resultFin", pokedex_cliente_log);

	log_trace(pokedex_cliente_log, "\tFINALIZO un MKDIR. Path: %s *****", path);

	return resultado;
}

static int osada_truncate(const char * path , off_t length)
{
	log_trace(pokedex_cliente_log, "\t***** Comienzo un TRUNCATE. Path: %s *****", path);

	// Envio el pedido de UNLINK
	enviarInt(clientSocket, TRUNCATE);

	// Envio el tamanio del path
	enviarInt(clientSocket, strlen(path));

	// Envio el path
	enviarMensaje(clientSocket, path);

	// Envio length
	enviarInt(clientSocket,length);

	// Espera la confirmacion del servidor para continuar...
	int * resultFin = myMalloc_int("osada_truncate resultFin", pokedex_cliente_log);
	int finFlag = recibirInt(clientSocket, resultFin, pokedex_cliente_log);
	if (finFlag == FIN_TRUNCATE)
	{
		log_trace(pokedex_cliente_log, "Finalizo el TRUNCATE - Cliente %d - Path %s", clientSocket, path);
	}
	else
	{
		log_error(pokedex_cliente_log, "ERROR EN EL TRUNCATE - Cliente %d - Path %s", clientSocket, path);
	}

	myFree(resultFin, "osada_truncate resultFin", pokedex_cliente_log);

	log_trace(pokedex_cliente_log, "\t***** Finalizo un read un TRUNCATE. Archivo: %s. *****", path);

	return 0;
}

static int osada_unlink(const char* path){

	log_trace(pokedex_cliente_log, "\t***** Comienzo un UNLINK. Path: %s *****", path);
	// Envio el pedido de UNLINK
	enviarInt(clientSocket, UNLINK);

	// Envio el tamanio del path
	enviarInt(clientSocket, strlen(path));

	// Envio el path
	enviarMensaje(clientSocket, path);

	// Recibir el resultado del borrado
	int * result = (int *)myMalloc_int("osada_unlink - result", pokedex_cliente_log);
	int status;
	status = recibirInt(clientSocket , result, pokedex_cliente_log);

	myFree(result, "osada_unlink - Cliente - result", pokedex_cliente_log);

	// Espera la confirmacion del servidor para continuar...
	int * resultFin = myMalloc_int("osada_unlink resultFin", pokedex_cliente_log);
	int finFlag = recibirInt(clientSocket, resultFin, pokedex_cliente_log);
	if (finFlag == FIN_UNLINK)
	{
		log_trace(pokedex_cliente_log, "Finalizo el UNLINK - Cliente %d - Path %s", clientSocket, path);
	}
	else
	{
		log_error(pokedex_cliente_log, "ERROR EN EL UNLINK - Cliente %d - Path %s", clientSocket, path);
	}

	myFree(resultFin, "osada_unlink resultFin", pokedex_cliente_log);

	log_trace(pokedex_cliente_log, "\t***** Finalizo un read un UNLINK. Archivo: %s. Status: %d *****", path, status);

	return status;

}

static int osada_rmdir(const char* path)
{

	log_trace(pokedex_cliente_log, "\t***** Comienzo un RMDIR. Path: %s *****", path);
	// Envio el pedido de RMDIR
	enviarInt(clientSocket, RMDIR);

	// Envio el tamanio del path
	enviarInt(clientSocket, strlen(path));

	// Envio el path
	enviarMensaje(clientSocket, path);

	// Espera la confirmacion del servidor para continuar...
	int * resultFin = myMalloc_int("osada_rmdir resultFin", pokedex_cliente_log);
	int finFlag = recibirInt(clientSocket, resultFin, pokedex_cliente_log);
	if (finFlag == FIN_RMDIR)
	{
		log_trace(pokedex_cliente_log, "Finalizo el RMDIR - Cliente %d - Path %s", clientSocket, path);
	}
	else
	{
		log_error(pokedex_cliente_log, "ERROR EN EL RMDIR - Cliente %d - Path %s", clientSocket, path);
	}

	myFree(resultFin, "osada_rmdir resultFin", pokedex_cliente_log);

	log_trace(pokedex_cliente_log, "\t***** Finalizo un read un RMDIR. Archivo: %s. *****", path);

	return 0;

}

static int osada_utimens (const char * path, const struct timespec tiempo[2])
{

	log_trace(pokedex_cliente_log, "\t***** Comienzo un UTIMENS. Path: %s *****", path);
	// Envio el pedido de RMDIR
	enviarInt(clientSocket, RMDIR);

	// Envio el tamanio del path
	enviarInt(clientSocket, strlen(path));

	// Envio el path
	enviarMensaje(clientSocket, path);

	// Envio el tiempo de modificacion
	enviarInt(clientSocket, tiempo[1].tv_sec);

	// Espera la confirmacion del servidor para continuar...
	int * resultFin = myMalloc_int("osada_utimens resultFin", pokedex_cliente_log);
	int finFlag = recibirInt(clientSocket, resultFin, pokedex_cliente_log);
	if (finFlag == FIN_UTIME)
	{
		log_trace(pokedex_cliente_log, "Finalizo el UTIMENS - Cliente %d - Path %s", clientSocket, path);
	}
	else
	{
		log_error(pokedex_cliente_log, "ERROR EN EL UTIMENS - Cliente %d - Path %s", clientSocket, path);
	}

	myFree(resultFin, "osada_utimens resultFin", pokedex_cliente_log);

	log_trace(pokedex_cliente_log, "\t***** Finalizo un read un UTIMENS. Archivo: %s. *****", path);

	return 0;

}

static int osada_rename(const char* oldPath, const char* newPath)
{

	log_trace(pokedex_cliente_log, "\t***** Comienzo un Rename. oldPath: %s *****", oldPath);
	// Envio el pedido de RMDIR
	enviarInt(clientSocket, RENAME);

	// Envio el tamanio del path
	enviarInt(clientSocket, strlen(oldPath));

	// Envio el path
	enviarMensaje(clientSocket, oldPath);

	// Envio el tamanio del path
	enviarInt(clientSocket, strlen(newPath));

	// Envio el path
	enviarMensaje(clientSocket, newPath);

	// Espera la confirmacion del servidor para continuar...
	int * resultFin = myMalloc_int("osada_rmdir resultFin", pokedex_cliente_log);
	int finFlag = recibirInt(clientSocket, resultFin, pokedex_cliente_log);
	if (finFlag == FIN_RENAME)
	{
		log_trace(pokedex_cliente_log, "Finalizo el RENAME - Cliente %d - Path %s", clientSocket, oldPath);
	}
	else
	{
		log_error(pokedex_cliente_log, "ERROR EN EL RENAME - Cliente %d - Path %s", clientSocket, oldPath);
	}

	myFree(resultFin, "osada_rename resultFin", pokedex_cliente_log);

	log_trace(pokedex_cliente_log, "\t***** Finalizo un read un RENAME. Archivo: %s. *****", newPath);

	return 0;

}

int CrearArchivoDirectorio(const char * path, int codigoPedido, int codigoTipo)
{
		// Envio el pedido de Creacion
		enviarInt(clientSocket, codigoPedido);

		// Envio el tamanio del path
		enviarInt(clientSocket, strlen(path));

		// Envio el path
		enviarMensaje(clientSocket, path);

		// Enviar el codigo: ARCHIVO o DIRECTORIO
		enviarInt(clientSocket, codigoTipo);

		//Esperar la respuesta
		int * result = (int *)myMalloc_int("MKDIR o CREATE - result", pokedex_cliente_log);
		int resultado;
		resultado = recibirInt(clientSocket , result, pokedex_cliente_log);

		// Libero
		myFree(result, "MKDIR o CREATE  - Cliente - result", pokedex_cliente_log);

		return resultado;

}
