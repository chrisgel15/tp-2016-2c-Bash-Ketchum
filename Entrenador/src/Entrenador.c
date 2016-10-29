#include "Entrenador.h"

//Log
t_log *entrenador_log;

//Configs
t_config * metadata;

t_entrenador *entrenador;
char *ruta_pokedex;
bool fin_prog;

//Socket Mapa
int socket_mapa;

//Hoja de Viaje
char ** hojaDeViaje ;

//Nombre del entrenador
char *nombre_entrendor;

//Estructuras para el manejo de la posicion
t_posicion_pokenest *posicion_pokenest;
t_posicion_mapa *posicion_mapa;


// Funciones utilitarias //
void init_datos_entrenador(void);



int main(int argc, char **argv) {

	fin_prog = false;
	nombre_entrendor = NULL;
	ruta_pokedex = NULL;


	if (argv[1] == NULL || argv[2] == NULL){
		perror("Se necesita el nombre del entrenador y la ruta de la Pokedex!");
		exit(1);
	}

	nombre_entrendor = argv[1];
	ruta_pokedex = argv[2];

	posicion_pokenest =  inicializar_posicion_pokenest();
	posicion_mapa =inicializar_posicion_entrenador();

	// Obtiene el archivo de metadata del entrenador.
	metadata = get_entrenador_metadata(ruta_pokedex, nombre_entrendor);
	init_datos_entrenador();

	//Obtiene hoja de viaje del archivo metadata del entrenador
	hojaDeViaje = get_entrenador_hoja_de_viaje(metadata);
	// Creacion del Log
	//char *log_level = config_get_string_value(mapa_log , LOG_LEVEL);
	char *log_level = "INFO";
	entrenador_log = CreacionLogWithLevel(log_nombre, programa_nombre, log_level);
	log_info(entrenador_log, "Se ha creador el Log para el Entrenador.");

	//Manejo de System Calls
	signal(SIGTERM, system_call_catch);
	if(fin_prog)
		return EXIT_SUCCESS;
	signal(SIGUSR1, system_call_catch);

	//printf("Bienvenido Entrenador %s! \n", nombre_entrendor);

	/*socket_mapa = conectar_mapa(ruta_pokedex,hojaDeViaje[0]);
	if(socket_mapa == 0){
		perror("Ocurrio un error al intentarse conectar al Mapa.");
		exit(1);
	}*/

	//handshake(nombre_entrendor);

	//Creo el Hilo que va a esperar los mensajes del Servidor
	//pthread_t chat_entrenadores;
	//pthread_create(&chat_entrenadores, NULL, (void *) recibir_mensajes, NULL);

	//Hilo para recorrer Hoja de Viaje
	//pthread_t entrenador_hojaDeViaje;
	//pthread_create(&entrenador_hojaDeViaje, NULL, (void *) recorrer_hojaDeViaje, ruta_pokedex);

	//Comieza el viaje del Entreandor
	recorrer_hojaDeViaje(ruta_pokedex);



	//printf("Adios Entrenador Pokemon!. \n");
	//Liberar memoria dinamica -> TODO: encapsular en funcion
	free(entrenador);
	return EXIT_SUCCESS;
}


//Funcion encargada de Capturar las llamadas de Sistema
void system_call_catch(int signal){

	char c;
	switch(signal){
	case SIGTERM:
		log_info(entrenador_log, "Se ha enviado la señal SIGTERM. Se le restará una vida al Entrenador.");

		if (!entrenador->vidas) {
			printf("No te quedan más vidas para continuar con tu aventura POKEMON, querés reintentar?: (Y/N)\n");
			printf("La cantidad de reintentos hasta el momento es %d", entrenador->reintentos);

			switch (c = getchar()) {
			case 'Y':
			case 'y':
				entrenador->reintentos++;
				// Reiniciar hoja de viaje;
				recorrer_hojaDeViaje(ruta_pokedex);
				// Borrar medallas obtenidas;
				// Borrar pokemones obtenidos;
				break;
			case 'N':
			case 'n':
				// Cerrar conexion, cerrar proceso, abandonar juego;
				printf("Abandonaste el juego, se cerrará la conexión y terminará el proceso");
				close(socket_mapa);
				fin_prog = true;
				break;
			default:
				break;
			}
		} else
			entrenador->vidas--;
		break;

	case SIGUSR1:
		log_info(entrenador_log, "Se ha enviado la señal SIGUSR1. Se le sumará una vida al Entrenador.");
		entrenador -> vidas ++;
		break;

	case SIGKILL:
		fin_prog = true;
		break;

	default:
		break;
	}
}

//Funcion para primer Check Point que se va a encargar de recibir mensajes del Servidor
//void recibir_mensajes(){
//	int tamanio_mensaje_recibido;
//	int *result = malloc(sizeof(u_int32_t));
//	char *mensaje_recibio = NULL;
//
//	//A la espera de recibir mensajes del servidor
//	while(1){
//		tamanio_mensaje_recibido = recibirInt(socket_mapa, result, entrenador_log);
//		if(*result > 0){
//			mensaje_recibio = malloc(sizeof(char) * tamanio_mensaje_recibido);
//			recibirMensaje(socket_mapa, mensaje_recibio, tamanio_mensaje_recibido, entrenador_log);
//			printf("%s \n", mensaje_recibio);
//		} else {
//			perror("Ocurrio un error al intentar recibir un mensaje del Servidor. FIN!.");
//			exit(0);
//		}
//	}
//}

int conectar_mapa(char* ruta_pokedex, char *mapa){
	ltn_sock_addinfo *ltn_cliente_entrenador;
	t_config * metadata_mapa = malloc(sizeof(t_config));
	metadata_mapa = get_mapa_metadata(ruta_pokedex , mapa);
	char *mapa_puerto = string_itoa(get_mapa_puerto(metadata_mapa));
	char *mapa_ip = get_mapa_ip( metadata_mapa);
	ltn_cliente_entrenador = createClientSocket(mapa_ip, mapa_puerto);
	//TODO REVISAR FREE
	//free(metadata_mapa);
	//free(mapa_puerto);
	//free(mapa_ip);
	int socket_mapa = doConnect(ltn_cliente_entrenador);
	return socket_mapa;
}

void solicitar_posicion_pokenest(char *pokemon){
	int *result = malloc(sizeof(int));
	int posicion_x, posicion_y;
	enviarInt(socket_mapa, UBICACION_POKENEST);
	enviarInt(socket_mapa, (sizeof(char) * 2));
	enviarMensaje(socket_mapa,pokemon);
	posicion_x = recibirInt(socket_mapa, result, entrenador_log);
	posicion_y = recibirInt(socket_mapa, result, entrenador_log);

	//Actualizo la posicion del proximo pokenest
	actualizar_posicion_pokenest(posicion_pokenest, posicion_x, posicion_y);
	free(result);
}


void capturar_pokemon(char *nombre_pokemon){
	int *result = malloc(sizeof(int));
	int codigo;
	enviarInt(socket_mapa, ATRAPAR_POKEMON);
	enviarInt(socket_mapa, 2);
	enviarMensaje(socket_mapa, nombre_pokemon);
	codigo = recibirInt(socket_mapa, result, entrenador_log);

	//TODO COPIAR ARCHIVO.DAT A DIRECTORIO BILL
	
}


int avanzar_hacia_pokenest(){
	int movimiento_x, movimiento_y;
	int mensaje_recibido;
	int *result = malloc(sizeof(u_int32_t));

	enviarInt(socket_mapa, AVANZAR_HACIA_POKENEST);

	//Me muevo en X
	movimiento_x = moverse_en_mapa_eje_x(posicion_mapa, posicion_pokenest);
	//enviarInt(socket_mapa, movimiento_x);
	enviarInt(socket_mapa, posicion_mapa->x);

	//mensaje_recibido = recibirInt(socket_mapa, result, entrenador_log);

//	if(*result < 0 && mensaje_recibido != ACCION_REALIZADA){
//		perror("Ocurrio un error al intentar recibir un mensaje del Mapa.");
//		exit(0);
//	}

	//Me muevo en Y
	movimiento_y = moverse_en_mapa_eje_y(posicion_mapa, posicion_pokenest);
	//enviarInt(socket_mapa, movimiento_y);
	enviarInt(socket_mapa, posicion_mapa->y);

	//mensaje_recibido = recibirInt(socket_mapa, result, entrenador_log);

//	if(*result < 0 && mensaje_recibido != ACCION_REALIZADA){
//		perror("Ocurrio un error al intentar recibir un mensaje del Mapa.");
//		exit(0);
//	}
//
//	free(result);

	//Valida si el entrenador no se tiene que mover más, si es asi se devuelve 1, de lo contrario 0
	if(movimiento_x == 0 && movimiento_y == 0){
		return 1;
	} else {
		return 0;
	}
}


bool objetivoCumplido(int posHojaDeViaje, int posPokenest){

	char **pokenests= get_entrenador_objetivos_por_mapa(metadata, hojaDeViaje[posHojaDeViaje]);
	return (pokenests[posPokenest+1]==NULL);
}


void terminarObjetivo(){

	int *result = malloc(sizeof(int));
	char *ruta_medalla = NULL;
	int tamanio_texto;

	recibirInt(socket_mapa, result, entrenador_log);//Espero TURNO_CONCEDIDO
	enviarInt(socket_mapa,OBJETIVO_CUMPLIDO);

	//Recibo la ruta de medalla
	tamanio_texto = recibirInt(socket_mapa, result, entrenador_log);
	ruta_medalla = malloc(sizeof(char) * tamanio_texto);
	recibirMensaje(socket_mapa, ruta_medalla, tamanio_texto, entrenador_log);
	//TODO COPIAR MEDALLA AL DIRECTORIO /ENTRENADOR/NOMBRE/MEDALLAS/

	//TODO INFORMAR POR PANTALLA TIEMPOS Y CANTIDAD DE DEADLOCKS

}


void recorrer_hojaDeViaje(char * ruta_pokedex) {
	int posHojaDeViaje = 0; //Indice para recorrer la Hoja de Viaje
	int posObjetivoPorMapa = 0; //Indice para recorrer los Objetivos por Mapa
	int *result = malloc(sizeof(int));
	int turnoConcedido;
	int estado = CONECTARSE_MAPA;
	char **objetivosPorMapa;

	while (hojaDeViaje[posHojaDeViaje]!= NULL){

		if(estado == CONECTARSE_MAPA){
			socket_mapa = conectar_mapa(ruta_pokedex, hojaDeViaje[posHojaDeViaje]);
			if(socket_mapa == 0){
				perror("Ocurrio un error al intentarse conectar al Mapa.");
				exit(1);
			}
			handshake();
		}

		estado = UBICACION_POKENEST;
		posObjetivoPorMapa = 0;
		objetivosPorMapa = get_entrenador_objetivos_por_mapa(metadata, hojaDeViaje[posHojaDeViaje]);

		while(estado != OBJETIVO_CUMPLIDO){

				//turnoConcedido = recibirInt(socket_mapa, result, entrenador_log);

				//if(*result > 0 && turnoConcedido == TURNO_CONCEDIDO){

					switch(estado){
						case UBICACION_POKENEST:
							//solicitar_posicion_pokenest(metadata, hojaDeViaje[posHojaDeViaje], posHojaDeViaje);
							solicitar_posicion_pokenest(objetivosPorMapa[posHojaDeViaje]);
							estado = AVANZAR_HACIA_POKENEST;
							break;
						case AVANZAR_HACIA_POKENEST:
							if (avanzar_hacia_pokenest() == 1){
								estado = ATRAPAR_POKEMON;
							} else {
								estado = AVANZAR_HACIA_POKENEST;
							}
							break;
						case ATRAPAR_POKEMON:
							capturar_pokemon(/*"P"*/objetivosPorMapa[posObjetivoPorMapa]);
							if (objetivoCumplido(/*0,0*/posHojaDeViaje,posObjetivoPorMapa)){
								estado = OBJETIVO_CUMPLIDO;
							} else {
								//posObjetivoPorMapa++;
								posObjetivoPorMapa = 0;
								estado = UBICACION_POKENEST;
							}
							break;
						default:
							log_error(entrenador_log, "Se ha producido un error al intentar realizar una accion del Entrenador.");
							break;
					}
//			} else {
//				perror("Ocurrio un error al intentarse interactuar con el Mapa.");
//				exit(0);
//			}
		}

		posHojaDeViaje++;

		if (hojaDeViaje[posHojaDeViaje] == NULL){
				printf("TE CONVERTISTE EN UN ENTRENADOR POKEMON!. \n");
				terminarObjetivo();
				//TODO DESCONECTARSE
		} else {
			/*socket_mapa = conectar_mapa(ruta_pokedex, hojaDeViaje[posHojaDeViaje]);
			if(socket_mapa == 0){
				perror("Ocurrio un error al intentarse conectar al Mapa.");
				exit(1);
			}
			handshake();*/
			estado = CONECTARSE_MAPA;
		}
	}
}


void handshake(){

		enviarInt(socket_mapa, SOY_ENTRENADOR);
		int size_nombre = (int) strlen(nombre_entrendor);
		//Envio el nombre del Entrenador
		enviarInt(socket_mapa, size_nombre);
		enviarMensaje(socket_mapa, nombre_entrendor);

		//Envio el caracter representante en el mapa
		enviarInt(socket_mapa, 2);
		char * simbolo = get_entrenador_simbolo(metadata);
		enviarMensaje(socket_mapa, simbolo);
		ingresar_a_nuevo_mapa(posicion_mapa);
}

void init_datos_entrenador(){
	entrenador = malloc(sizeof(t_entrenador));
	entrenador -> vidas = get_entrenador_vidas(metadata);
	entrenador -> reintentos = get_entrenador_reintentos(metadata);

}
