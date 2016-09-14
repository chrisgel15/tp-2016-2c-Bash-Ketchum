#include "Entrenador.h"

//Log
t_log *entrenador_log;

//Configs
t_config * metadata;

//Socket Mapa
int socket_mapa;

char ** hojaDeViaje ;

int main(int argc, char **argv) {
	int i =0; // indice para recorrer la hojaDeViaje TODO @GI (mejorar la idea );

	char *nombre_entrendor = NULL;
	char * ruta_pokedex = NULL;

	if (argv[1] == NULL || argv[2] == NULL){
		perror("Se necesita el nombre del entrenador y la ruta de la Pokedex!");
		exit(1);
	}

	nombre_entrendor = argv[1];
	ruta_pokedex = argv[2];

	// Obtiene el archivo de metadata del entrenador.
	metadata = get_entrenador_metadata(ruta_pokedex, nombre_entrendor);

	//Obtiene hoja de viaje del archivo metadata del entrenador
	hojaDeViaje=get_entrenador_hoja_de_viaje(metadata);



	// Creacion del Log
	//char *log_level = config_get_string_value(mapa_log , LOG_LEVEL);
	char *log_level = "INFO";
	entrenador_log = CreacionLogWithLevel(log_nombre, programa_nombre, log_level);
	log_info(entrenador_log, "Se ha creador el Log para el Entrenador.");

	//Manejo de System Calls
	signal(SIGTERM, system_call_catch);

	printf("Bienvenido Entrenador %s! \n", nombre_entrendor);


	socket_mapa=conectar_mapa( hojaDeViaje[i]);


	if(socket_mapa == 0){
		perror("Ocurrio un error al intentarse conectar al Mapa.");
		exit(1);
	}

	//Handshake con el Mapa
	enviarInt(socket_mapa, SOY_ENTRENADOR);
	int size_nombre = (int) strlen(nombre_entrendor);
	//Envio el nombre del Entrenador
	enviarInt(socket_mapa, size_nombre);
	enviarMensaje(socket_mapa, nombre_entrendor);

	//Envio el caracter representante en el mapa
	enviarInt(socket_mapa, 2);
	char * simbolo = get_entrenador_simbolo(metadata);
	enviarMensaje(socket_mapa, simbolo);

	//Creo el Hilo que va a esperar los mensajes del Servidor
	pthread_t chat_entrenadores;
	pthread_create(&chat_entrenadores, NULL, (void *) recibir_mensajes, NULL);

	//Metodo para el Primer checkpoint
	int seguir_enviando = 1;
	int tamanio_mensaje;
	char texto[2000];
	char *fin = "Chau";

	printf("Bienvenido al Servidor de Chats de Entrenadores Pokemon. Por favor, comienza a escribir mensajes: \n");
	while(seguir_enviando){

		fgets (texto, 20000, stdin);
		if(strcmp(texto, fin) == 0){
			seguir_enviando = 0;
		} else {
			//Envio el Mensaje
			enviarInt(socket_mapa, ENVIAR_MENSAJE);
			tamanio_mensaje = 2000;
			enviarInt(socket_mapa, tamanio_mensaje);
			enviarMensaje(socket_mapa, texto);
		}
	}

	printf("Adios Entrenador Pokemon!. \n");
	return EXIT_SUCCESS;
}


//Funcion encargada de Capturar las llamadas de Sistema
void system_call_catch(int signal){

	if (signal == SIGTERM){
		log_info(entrenador_log, "Se ha enviado la señal SIGTERM. Se le sumará una vida al Entrenador.");
		//TODO: Sumar vida
	}
}

//Funcion para primer Check Point que se va a encargar de recibir mensajes del Servidor
void recibir_mensajes(){
	int tamanio_mensaje_recibido;
	int *result = malloc(sizeof(u_int32_t));
	char *mensaje_recibio = NULL;

	//A la espera de recibir mensajes del servidor
	while(1){
		tamanio_mensaje_recibido = recibirInt(socket_mapa, result, entrenador_log);
		if(*result > 0){
			mensaje_recibio = malloc(sizeof(char) * tamanio_mensaje_recibido);
			recibirMensaje(socket_mapa, mensaje_recibio, tamanio_mensaje_recibido, entrenador_log);
			printf("%s \n", mensaje_recibio);
		} else {
			perror("Ocurrio un error al intentar recibir un mensaje del Servidor. FIN!.");
			exit(1);
		}
	}
}

int  conectar_mapa(char mapa){
		ltn_sock_addinfo *ltn_cliente_entrenador;
		char *mapa_puerto = "8000"; //TODO: Borrar cuando se implemente el archivo de Configuracion
		char *mapa_ip = "127.0.0.1";
		ltn_cliente_entrenador = createClientSocket(mapa_ip, mapa_puerto);
		return doConnect(ltn_cliente_entrenador);

}

void solicitar_posicion_pokenest(t_config* metadata,char *mapa,int posPokenest){

	char ** pokenests = get_entrenador_objetivos_por_mapa(metadata, mapa);
	enviarInt(socket_mapa,UBICACION_POKENEST);
	enviarMensaje(socket_mapa, pokenests[posPokenest]);

}





