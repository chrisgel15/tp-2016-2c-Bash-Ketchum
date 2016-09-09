
#include "Mapa.h"

//Estructuras para el Manejo de Entrenadores
t_queue *entrenadores_listos;
t_list *entrenadores; //TODO: Ver si puede llegar a servir
t_list *pokenests;

//lista de items a dibujar en el mapa
t_list* items;
t_datos_mapa* datos_mapa;

//Log
t_log *mapa_log;

pthread_mutex_t mutex_desplaza_x;
pthread_mutex_t mutex_desplaza_y;


int main(void) {

	// Creacion del Log
	//char *log_level = config_get_string_value(mapa_log , LOG_LEVEL);
	char *log_level = "TRACE";
	mapa_log = CreacionLogWithLevel(log_nombre, programa_nombre, log_level);
	mapa_log->is_active_console = false;
	log_info(mapa_log, "Se ha creado el Log para el Mapa.");


	//Inicializamos Estructuras
	inicializar_estructuras();

	//Creamos el Servidor de Entrenadores
	char *puerto_entrenadores = "8000"; //TODO: Sacar cuando se levanten de la configuracion
	int listener_entrenadores;
	ltn_sock_addinfo* ltn_entrenadores;

	ltn_entrenadores = createSocket(puerto_entrenadores);
	listener_entrenadores = doBind(ltn_entrenadores);
	listener_entrenadores = doListen(listener_entrenadores, 100);

	ltn_fd_sets *fd_sets_entrenadores = createFdSets(listener_entrenadores);

	log_info(mapa_log, "A la espera de Entrenadores Pokémon.");


	dibujar_mapa_vacio(items);

	//Espero conexiones y pedidos de Entrenadores
	while(1){
		fd_sets_entrenadores->readFileDescriptorSet = fd_sets_entrenadores->masterSet;

		if (select(fd_sets_entrenadores->maxFileDescriptorNumber + 1,
						&(fd_sets_entrenadores->readFileDescriptorSet),
						&(fd_sets_entrenadores->writeFileDescriptorSet),
						NULL, NULL) == -1){
			/*perror*/log_error(mapa_log,"Se ha producido un error al intentar atender las peticiones de los Entrenadores.");
			exit(1);
		}

		fd_sets_entrenadores = checkReads(fd_sets_entrenadores, atender_entrenador, despedir_entrenador, mapa_log);
	}

	return EXIT_SUCCESS;
}

/********* FUNCIONES DE INICIALIZACION *********/
void inicializar_estructuras(){
	entrenadores_listos = queue_create();
	entrenadores = list_create();
	pokenests= list_create();
	items = list_create();
	datos_mapa = malloc(sizeof(t_datos_mapa));
	datos_mapa->items = items;
	datos_mapa->entrenador = NULL;
	pthread_mutex_init(&mutex_desplaza_x,NULL);
	pthread_mutex_init(&mutex_desplaza_y,NULL);

}

/********* FUNCIONES PARA RECIBIR PETICIONES DE LOS ENTRENADORES *********/
void atender_entrenador(int fd_entrenador, int codigo_instruccion){

	switch(codigo_instruccion){
		case SOY_ENTRENADOR:
			recibir_nuevo_entrenador(fd_entrenador);
			break;
		case ENVIAR_MENSAJE:
			recibir_mensaje_entrenador(fd_entrenador);
			break;
		/*case UBICACION_POKENEST:
			enviar_posicion_pokenest(id);
			break;
		case AVANZAR_HACIA_POKENEST:
			mover_entrenador();
			break;
		case ATRAPAR_POKEMON:
			conceder_pokemon();
			break;
		case OBJETIVO_CUMPLIDO();
			conceder_medalla();
			*/
		default:
			log_error(mapa_log, "Se ha producido un error al intentar atender a la peticion del Entrenador.");
			break;
	}
}

void recibir_nuevo_entrenador(int fd){
	t_entrenador *entrenador = malloc(sizeof(t_entrenador));
	t_posicion* posicion = malloc(sizeof(t_posicion));
	entrenador->posicion = posicion;

	int *result = malloc(sizeof(int));
	char *nombre = NULL;
	char *caracter = NULL;
	int tamanio_texto;

	//Recibo el nombre
	tamanio_texto = recibirInt(fd, result, mapa_log);
	nombre = malloc(sizeof(char) * tamanio_texto);
	recibirMensaje(fd, nombre, tamanio_texto, mapa_log);

	//Recibo el Caracter
	tamanio_texto = recibirInt(fd, result, mapa_log);
	caracter = malloc(sizeof(char) * tamanio_texto);
	recibirMensaje(fd, caracter, tamanio_texto, mapa_log);

	//Cargo la estructura del Entrenador con los datos recibidos por Socket
	//La posicion inicial es (0;0)
	entrenador->fd = fd;
	entrenador->nombre = nombre;
	entrenador->caracter =  *caracter;
	entrenador->posicion->x = POSICION_INICIAL_X;
	entrenador->posicion->y = POSICION_INICIAL_Y;

	datos_mapa->entrenador = entrenador;

	list_add(entrenadores, entrenador);

	/* Pruebo dibujar el mapa con la posicion inical del entrenador*/

	posicionar_entrenador_en_mapa(datos_mapa);

	/**** Hilo para manejar el trazado del mapa ****/
		pthread_t trazado_mapa;
		pthread_create(&trazado_mapa, NULL, (void*)mover_entrenador_hacia_recurso, (void*)datos_mapa);
		/***********************************************/

	//mover_entrenador_hacia_recurso(datos_mapa);

	/*****************************************************************/

	//printf("Bienvenido Entrenador %s N° %d. \n", entrenador->nombre, fd);
	log_info(mapa_log,"Bienvenido Entrenador %s N° %d. \n", entrenador->nombre, fd);
}

void recibir_mensaje_entrenador(int fd){
	int tamanio_texto;
	int *result = malloc(sizeof(int));
	char *texto = NULL;
	char *texto_enviar = NULL;
	t_entrenador *entrenador_origen = buscar_entrenador(fd);
	int cantidad_entrenadores = list_size(entrenadores);

	//Recibo el mensaje
	tamanio_texto = recibirInt(fd, result, mapa_log);
	texto = malloc(sizeof(char) * tamanio_texto);
	recibirMensaje(fd, texto, tamanio_texto, mapa_log);

	//printf("Mensaje recibido del socket %d: %s", fd, texto);
	log_info(mapa_log, "Mensaje recibido del socket %d: %s", fd, texto);

	//Enviamos el mensaje a todos los Entrenadores
	int i;
	for(i = 0; i < cantidad_entrenadores; i++){
		t_entrenador *entrenador = (t_entrenador *)list_get(entrenadores, i);

		if(entrenador->fd != fd){
			texto_enviar = string_new();
			string_append(&texto_enviar, entrenador_origen->nombre);
			string_append(&texto_enviar, ": ");
			string_append(&texto_enviar, texto);
			tamanio_texto = (int) strlen(texto_enviar);
			enviarInt(entrenador->fd, tamanio_texto);
			enviarMensaje(entrenador->fd, texto_enviar);
		}
	}

	free(result);
	free(texto);
	free(texto_enviar);
}

void despedir_entrenador(int fd_entrenador, int codigo_instruccion){

}

t_entrenador *buscar_entrenador(int fd){
	int cantidad_entrenadores = list_size(entrenadores);
	int i;

	for(i = 0; i < cantidad_entrenadores; i++){
		t_entrenador * entrenador = (t_entrenador *)list_get(entrenadores, i);

		if(entrenador->fd == fd){
			return entrenador;
		}
	}

	return NULL;
}

void conceder_pokemon(){

}

void conceder_medalla(){

}
