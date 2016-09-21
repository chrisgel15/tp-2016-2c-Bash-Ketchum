/*
 * Srdf.h
 *
 *  Created on: 12/9/2016
 *      Author: utnso
 */

#ifndef SRDF_H_
#define SRDF_H_

#include <stdio.h>
#include <stdlib.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <curses.h>
#include <nivel.h>
#include <tad_items.h>
#include <unistd.h>
#include <math.h>
#include <stdbool.h>

typedef struct{
	char* nombre;
	char id;
	int posx;
	int posy;
	int pokenx;
	int pokeny;
}t_entrenador;

void srdf(t_list*);


#endif /* SRDF_H_ */
