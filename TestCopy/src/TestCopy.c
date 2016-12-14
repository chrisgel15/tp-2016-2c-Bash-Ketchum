/*
 ============================================================================
 Name        : TestCopy.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>

int main(void) {

	printf("Comienzo el copy...");

	// este seria el path del archivo.
	char * old = "/home/utnso/osada_mount/hello.txt";

	// este es el path destino
	char * new = "/home/utnso/osada_mount/dir1";

	// este cmd contiene un string con la ejecucion
	char * cmd = malloc(sizeof(char)*200);

	// En CMD se guarda el comando
	// en /bin/cp se encuentra el comando copy.
	// -p es para preservar los permisos y el timestamp
	// Luego van los dos path de los archivos
	sprintf( cmd, "/bin/cp -p \'%s\' \'%s\'", old, new);

	// Finalmente se ejecuta de esta forma...
	system(cmd);

	return 1;
}
