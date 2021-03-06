/*
 * instructionCodes.h
 *
 *  Created on: 17/7/2016
 *      Author: utnso
 */

#ifndef COMMUNICATIONS_INSTRUCTIONCODES_H_
#define COMMUNICATIONS_INSTRUCTIONCODES_H_

// Configs
#define MAPA_PUERTO_ESCUCHA "MAPA_PUERTO_ESCUCHA"
#define POKEDEX_SERVIDOR_PUERTO_ESCUCHA "POKEDEX_SERVIDOR_PUERTO_ESCUCHA"

#define ENTRENADOR_IP 			"ENTRENADOR_IP"
#define MAPA_IP 				"MAPA_IP"
#define POKEDEX_CLIENTE_IP 		"POKEDEX_CLIENTE_IP"
#define POKEDEX_SERVIDOR_IP 	"POKEDEX_SERVIDOR_IP"

#define LOG_LEVEL "LOG_LEVEL"

// Definiciones de los codigos de handshake para la comunicacion
#define SOY_ENTRENADOR		101
#define SOY_POKEDEX_CLIENTE	102

//Definiciones entre Entrenador y Mapa
#define ENVIAR_MENSAJE 			200
#define UBICACION_POKENEST	 	201
#define AVANZAR_HACIA_POKENEST 	202
#define ATRAPAR_POKEMON 		203
#define OBJETIVO_CUMPLIDO 		204
#define TURNO_CONCEDIDO  		205
#define OBJETIVO_NO_CUMPLIDO 	206
#define POKEMON_CONCEDIDO 		207
#define MUERTE			 		208
#define VICTORIA				209
#define DERROTA					210
#define DESCONECTADO 			9999

//Definciones entre Pokedex Clientes y Pokedex Servidor

/*Errores*/
#define NO_SE_RECIBIERON_DATOS 0

#endif /* COMMUNICATIONS_INSTRUCTIONCODES_H_ */
