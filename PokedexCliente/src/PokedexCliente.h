/*
 * PokedexCliente.h
 *
 *  Created on: 23/11/2016
 *      Author: utnso
 */

#ifndef POKEDEXCLIENTE_H_
#define POKEDEXCLIENTE_H_

#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <fuse.h>
#include <commons/log.h>
#include <commons/string.h>
#include <communications/ltnCommons.h>
#include <sys/types.h>
#include <sys/syscall.h>

#define PUERTO_POKEDEX_SERVIDOR "PUERTO_POKEDEX_SERVIDOR"
#define IP_POKEDEX_SERVIDOR "IP_POKEDEX_SERVIDOR"
#define PUNTO_MONTAJE_POKEDEX_SERVIDOR "PUNTO_MONTAJE_POKEDEX_SERVIDOR"

static int osada_getattr(const char *path, struct stat *stbuf);
static int osada_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);
static int osada_read(const char *path, char *buf, size_t size, off_t offset,struct fuse_file_info *fi);
static int osada_write (const char * path, const char * buf, size_t size, off_t off, struct fuse_file_info * fi);
static int osada_create (const char * path, mode_t mode, struct fuse_file_info * fi);
static int osada_mkdir (const char * path, mode_t mode);
static int osada_truncate(const char * filename , off_t length);
static int osada_unlink(const char* path);
static int osada_rmdir(const char* filename);
static int osada_utimens (const char * path, const struct timespec tiempo[2]);
static int osada_rename(const char* oldPath, const char* newPath);

int CrearArchivoDirectorio(const char * path, int codigoPedido, int codigoTipo);

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

static struct fuse_operations osada_oper = {
		.getattr = osada_getattr,
		.readdir = osada_readdir,
		.read = osada_read,
		.write = osada_write,
		.create = osada_create,
		.unlink = osada_unlink,
		.mkdir = osada_mkdir,
		.truncate = osada_truncate,
		.rmdir = osada_rmdir,
		.utimens = osada_utimens,
		.rename = osada_rename
};

// Variables globales
t_log * pokedex_cliente_log;
t_config* pokedex_cliente_config;

char * logLevel;
char * puertoServer;
char * ipServer;
ltn_sock_addinfo *ltn_pokedex_cliente;
int clientSocket;


#endif /* POKEDEXCLIENTE_H_ */
