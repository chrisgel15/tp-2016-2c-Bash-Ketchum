
#include "Mapa.h"

//Mutexs de Estructura de Estados
pthread_mutex_t mutex_entrenadores_listos;
pthread_mutex_t mutex_cola_bloqueados;

//Semaforos para sincronizar Hilos
sem_t sem_listos;

//Estructuras para el Manejo de Entrenadores
t_list *entrenadores_listos;
t_queue *entrenadores_bloqueados;
t_queue *entrenadores_ejecutando;

t_list *pokenests;

//lista de items a dibujar en el mapa
t_list* items;
t_datos_mapa* datos_mapa;

//Log
t_log *mapa_log;

pthread_mutex_t mutex_desplaza_x;
pthread_mutex_t mutex_desplaza_y;


int main(int argc, char **argv) {

	char *nombre_mapa = string_new();

	if (argv[1] == NULL){
		perror("Ingrese el Nombre del Mapa para volver a comenzar.");
		exit(1);
	}

	string_append(&nombre_mapa, argv[1]);

	// Creacion del Log
	//char *log_level = config_get_string_value(mapa_log , LOG_LEVEL);
	char *log_level = "TRACE";
	mapa_log = CreacionLogWithLevel(log_nombre, programa_nombre, log_level);
	mapa_log->is_active_console = false;
	log_info(mapa_log, "Se ha creado el Log para el Mapa.");


	//Inicializamos Estructuras
	inicializar_estructuras();

	//Manejo de System Calls
	signal(SIGUSR2, system_call_catch);

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

/********* FUNCIONES PARA EL MANEJO DE ESTRUCTURAS DE ESTADOS *********/

//Agrega un nuevo programa a la Cola de Listos
void agregar_entrenador_a_listos(t_entrenador *entrenador) {
	pthread_mutex_lock(&mutex_entrenadores_listos);
	list_add(entrenadores_listos, entrenador);
	queue_push(entrenadores_listos, (void *) entrenador);
	sem_post(&sem_listos);
	pthread_mutex_unlock(&mutex_entrenadores_listos);
}

//Remover entrenador segun Round Robin
t_entrenador *remover_entrenador_listo_por_RR(){
	t_entrenador *entrenador = NULL;
	pthread_mutex_lock(&mutex_entrenadores_listos);
	entrenador = (t_entrenador *) list_remove(entrenadores_listos, 0);
	pthread_mutex_unlock(&mutex_entrenadores_listos);
	return entrenador;
}

//Remover entrenador segun Algoritmo Shortest Remaining Distance First
t_entrenador *remover_entrenador_listo_por_SRDF(){

}

//Agrega un entrenador a la cola bloqueados
void agregar_entrenador_a_bloqueados(t_entrenador *entrenador){
	pthread_mutex_lock(&mutex_cola_bloqueados);
	queue_push(entrenadores_bloqueados, (void *) entrenador);
	pthread_mutex_unlock(&mutex_cola_bloqueados);
}

/********* FUNCIONES DE INICIALIZACION *********/
void inicializar_estructuras(){

	//Estructuras
	entrenadores_listos = list_create();
	pokenests= list_create();
	items = list_create();

	datos_mapa = malloc(sizeof(t_datos_mapa));
	datos_mapa->items = items;
	datos_mapa->entrenador = NULL;

	//Semaforos
	pthread_mutex_init(&mutex_desplaza_x, NULL);
	pthread_mutex_init(&mutex_desplaza_y, NULL);
	pthread_mutex_init(&mutex_entrenadores_listos, NULL);
	pthread_mutex_init(&mutex_cola_bloqueados, NULL);

	sem_init(&sem_listos, 0, 0);

	log_info(mapa_log, "Se inicializaron las Estructuras y los Semaforos.");
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
		//Estos metodos solo van a ser pedidos cuando se esta atendiendo al entrenador, el Entrenador no va a mandar estos mensajes por si solo
		/*case UBICACION_POKENEST:
			enviar_posicion_pokenest(fd_entrenador);
			break;
		case AVANZAR_HACIA_POKENEST:
			mover_entrenador();
			break;
		case ATRAPAR_POKEMON:
			entregar_pokemon(fd_entrenador);
			break;
		case OBJETIVO_CUMPLIDO();
			entregar_medalla();
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

	agregar_entrenador_a_listos(entrenador);

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
	int cantidad_entrenadores = list_size(entrenadores_listos);

	//Recibo el mensaje
	tamanio_texto = recibirInt(fd, result, mapa_log);
	texto = malloc(sizeof(char) * tamanio_texto);
	recibirMensaje(fd, texto, tamanio_texto, mapa_log);

	//printf("Mensaje recibido del socket %d: %s", fd, texto);
	log_info(mapa_log, "Mensaje recibido del socket %d: %s", fd, texto);

	//Enviamos el mensaje a todos los Entrenadores
	int i;
	for(i = 0; i < cantidad_entrenadores; i++){
		t_entrenador *entrenador = (t_entrenador *)list_get(entrenadores_listos, i);

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
	int cantidad_entrenadores = list_size(entrenadores_listos);
	int i;

	for(i = 0; i < cantidad_entrenadores; i++){
		t_entrenador * entrenador = (t_entrenador *)list_get(entrenadores_listos, i);

		if(entrenador->fd == fd){
			return entrenador;
		}
	}

	return NULL;
}

void entregar_pokemon(int fd ){

	t_entrenador* entrenador=malloc (sizeof(t_entrenador));
	entrenador=buscar_entrenador(fd);

	int tamanio_texto;
	int *result = malloc(sizeof(int));
	char *nombre_pokenest= NULL;

	//Recibo el mensaje TODO REUTILIZAR FUNCION RECIBIR_MENSAJE_ENTRENADOR
	tamanio_texto = recibirInt(fd, result, mapa_log);
	nombre_pokenest= malloc(sizeof(char) * tamanio_texto);
	recibirMensaje(fd, nombre_pokenest, tamanio_texto, mapa_log);
	free(result);

	//TODO @GI INCORPORAR SEMAFOROS PARA LAS POKENESTs
	ITEM_NIVEL * pokenest= malloc(sizeof(ITEM_NIVEL));
	pokenest= _search_item_by_id(items, nombre_pokenest);
	pokenest->quantity=pokenest->quantity-1;

	//TODO ELIMINAR ENTRENADOR DE LA COLA DE LISTOS
	agregar_entrenador_a_bloqueados(entrenador);

	free(pokenest);



}

void entregar_medalla(){

}

void enviar_posicion_pokenest(int fd ){
	int tamanio_texto;
	int *result = malloc(sizeof(int));
	char *nombre_pokenest = NULL;

	//Recibo el mensaje TODO REUTILIZAR FUNCION RECIBIR_MENSAJE_ENTRENADOR
	tamanio_texto = recibirInt(fd, result, mapa_log);
	free(result);
	nombre_pokenest= malloc(sizeof(char) * tamanio_texto);
	recibirMensaje(fd, nombre_pokenest, tamanio_texto, mapa_log);

	ITEM_NIVEL * pokenest= malloc(sizeof(ITEM_NIVEL));
	pokenest = _search_item_by_id(items, nombre_pokenest);
	enviarInt(fd,pokenest->posx);
	enviarInt(fd,pokenest->posy);
	free(pokenest);
	free(nombre_pokenest);

}

/********* FUNCION ENCARGADA DEL MANEJO DE LAS SYSTEM CALLS*********/
void system_call_catch(int signal){

	if (signal == SIGUSR2){
		log_info(mapa_log, "Se ha enviado la señal SIGUSR2. Se actualizarán las variables de Configuración.");
		/*TODO: Actualizar Algoritmo de planificación, valor de quantum, tiempo de retardo entre turnos y tiempo de chequeo de interbloqueo
		/Hacer esto cuando ya lo tengamos levantando desde el archivo de Configuración*/
	}
}
