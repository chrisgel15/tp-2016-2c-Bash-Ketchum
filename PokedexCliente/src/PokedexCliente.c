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
	pokedex_cliente_log = CreacionLogWithLevel("pokedex-cliente-log", "pokedex-cliente-program", "TRACE");

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

	log_trace(pokedex_cliente_log, "Estoy en GETATTR. FD %d.", clientSocket);

	memset(stbuf, 0, sizeof(struct stat));

	if (strstr(path, ".Trash") != NULL ||
				strstr(path, ".Trash-1000") != NULL ||
					strstr(path, "xdg-volume-info") != NULL	||
						strstr(path, "autorun.inf") != NULL )
			return -ENOENT;

	if (strcmp(path, "/" )== 0)
	{
		log_trace(pokedex_cliente_log, "Entro a '/'");
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	}
	else if (strcmp(path, "/directorio") == 0)
	{
		log_trace(pokedex_cliente_log, "Entro a '/directorio'");
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 1;
		stbuf->st_size = 0;
	}
	else
	{
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = 1;
	}

	return 0;



	log_trace(pokedex_cliente_log, "Fin GETATTR CORTO. FD %d", clientSocket);

	int res = 0;
//	int * directoryId = malloc(sizeof(char)*4);
//	*directoryId = -1;
	memset(stbuf, 0, sizeof(struct stat));

	if (strstr(path, ".Trash") != NULL ||
			strstr(path, ".Trash-1000") != NULL ||
				strstr(path, "xdg-volume-info") != NULL	||
					strstr(path, "autorun.inf") != NULL ||
						strstr(path, ".directory" != NULL ))
		return -ENOENT;

	if (strcmp(path, "/" )== 0)
	{
		log_trace(pokedex_cliente_log, "Entro a '/'");
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	}
	else
	{
		log_trace(pokedex_cliente_log, "Entro a 'ELSE'. Path: %s", path);

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

	log_trace(pokedex_cliente_log, "FIN GETATTR. FD %d.", clientSocket);



	return res;

}

static int osada_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
	// Enviar la señal READDIR

	log_trace(pokedex_cliente_log, "\n\nComienzo un READDIR");

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
	log_trace(pokedex_cliente_log, "\n\nFINALIZO un READDIR");
	return 0;
}

static int osada_read(const char *path, char *buf, size_t size, off_t offset,struct fuse_file_info *fi)
{
	// Enviar al servidor...
	return 0;
}

static int osada_write (const char * path, const char * buf, size_t size, off_t off, struct fuse_file_info * fi)
{
	// Reenviar al servidor
	return 0;
}

static int osada_create (const char * path, mode_t mode, struct fuse_file_info * fi)
{
	// Reenviar al servidor
	return 0;
}

static int osada_mkdir (const char * path, mode_t mode)
{
	// Reenviar al servidor
	return 0;
}

static int osada_truncate(const char * filename , off_t length)
{
	// Reenviar al servidor
	return 0;
}
