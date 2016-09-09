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
#include "Osada.h"

#define FS_SIZE 100
#define TABLA_ARCH 1024
#define HEADER 1

int main(void) {

	osada_header* header = malloc(sizeof(osada_header));

	FILE* fp = fopen("/home/utnso/osadaArchTest.bin", "r");

	uint32_t tamanio_fs_bloques = FS_SIZE * 1024 * 1024 / OSADA_BLOCK_SIZE;

	if (fread((void*) header, OSADA_BLOCK_SIZE, 1, fp) == 1) {
		printf("El filesystem es: %s\n", header->magic_number);
		printf("La version del FS es: %d\n", header->version);
		printf("El tamanio en bloques del FS es: %d\n", header->fs_blocks);
		printf("La cantidad del bloques para datos del FS es: %d\n",
				header->data_blocks);
		printf("El primer bloque de la tabla de asignacion del FS es: %d\n",
				header->allocations_table_offset);
		printf("El tamanio de la tabla de asignacion es: %d\n",
				(tamanio_fs_bloques - HEADER - (header->bitmap_blocks)
						- TABLA_ARCH) * 4 / OSADA_BLOCK_SIZE);
		printf("El tamanio en bloques del bitmap del FS es: %d\n",
				header->bitmap_blocks);

	}

	fclose(fp);
	free(header);
	return EXIT_SUCCESS;

}
