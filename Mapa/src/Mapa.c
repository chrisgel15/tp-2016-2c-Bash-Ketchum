
#include "Mapa.h"

//Mutexs de Estructura de Estados
pthread_mutex_t mutex_entrenadores_listos;
pthread_mutex_t mutex_cola_bloqueados;
//pthread_mutex_t mutex_entrenadores_ejecutando;
pthread_mutex_t mutex_pokenests;

//Semaforos para sincronizar Hilos
sem_t sem_listos;
sem_t *sem_entrenadores_bloqueados;
sem_t *sem_pokemones_disponibles;
sem_t sem_ejecutando;
sem_t sem_mensajes;

//Estructuras para el Manejo de Entrenadores
t_list *entrenadores_listos;
t_dictionary *entrenadores_bloqueados;
//t_list *entrenadores_ejecutando;
t_list *mensajes_entrenadores;

//t_list *pokenests;
//Lista de nombres de pokenests
char ** pokenests;
t_list *lista_pokenests;

//lista de items a dibujar en el mapa
t_list* items;
t_datos_mapa* datos_mapa;

//Log
t_log *mapa_log;

//metadata
t_config * metadata;

//Algoritmo
char* algoritmo;

//Quantum
int mapa_quantum;

//Retardo
int retardo;

//Interbloqueo
int interbloqueo;
t_list *lista_interbloqueo;

//Nombre de Mapa
char *nombre_mapa;
char *ruta_pokedex;

//Ids de Entrenadores
int entrenador_id = 0;

pthread_mutex_t mutex_desplaza_x;
pthread_mutex_t mutex_desplaza_y;

//SEMAFORO PARA CONTROLAR LA CANTIDAD DE RECURSOS DE POKENEST TODO DEBERIA HABER UNO POR CADA POKENEST?
pthread_mutex_t mutex_recursos_pokenest;


//****************************************************
// Prototipos de funciones internas

bool menor_distancia(t_entrenador*, t_entrenador*);
double distancia_a_pokenest(t_entrenador*);
void srdf(t_list*);
//****************************************************

int main(int argc, char **argv) {

	nombre_mapa = string_new();
	ruta_pokedex = string_new();

	if (argv[1] == NULL || argv[2] == NULL ){
		perror("Asegurese de ingresar el nombre del Mapa y la ruta del pokedex");
		exit(1);
	}

	string_append(&nombre_mapa, argv[1]);
	string_append(&ruta_pokedex, argv[2]);

	metadata = get_mapa_metadata(ruta_pokedex, nombre_mapa);

	// Creacion del Log
	//char *log_level = config_get_string_value(mapa_log , LOG_LEVEL);
	char *log_level = "TRACE";
	mapa_log = CreacionLogWithLevel(log_nombre, programa_nombre, log_level);
	mapa_log->is_active_console = false;
	log_info(mapa_log, "Se ha creado el Log para el Mapa.");


	//Inicializamos el Listado de los Pokenests con sus respectivos Pokemons
	lista_pokenests = get_listado_pokenest(argv[2] , nombre_mapa);
//
	//Inicializamos Estructuras
	inicializar_estructuras();

	//Manejo de System Calls
	signal(SIGUSR2, system_call_catch);

	//Creamos el Servidor de Entrenadores
	int puerto_entrenadores= get_mapa_puerto(metadata);
	int listener_entrenadores;
	ltn_sock_addinfo* ltn_entrenadores;

	ltn_entrenadores = createSocket(string_itoa(puerto_entrenadores));
	listener_entrenadores = doBind(ltn_entrenadores);
	listener_entrenadores = doListen(listener_entrenadores, 100);

	ltn_fd_sets *fd_sets_entrenadores = createFdSets(listener_entrenadores);

	log_info(mapa_log, "A la espera de Entrenadores Pokémon.");

	//Algoritmo
	algoritmo = get_mapa_algoritmo(metadata);

	//Quantum
	mapa_quantum = get_mapa_quantum(metadata);

	//Retardo
	retardo = get_mapa_retardo(metadata);

	//Interbloqueo
	interbloqueo = get_mapa_tiempo_deadlock(metadata);

	//Inicializamos la Interfaz Grafica
	inicializar_mapa(items, lista_pokenests, nombre_mapa);

	//Hilo Planificador
	pthread_t planificador;
	pthread_create(&planificador, NULL, (void *) administrar_turnos, NULL);

	//Espero conexiones y pedidos de Entrenadores
	while(1){
		fd_sets_entrenadores->readFileDescriptorSet = fd_sets_entrenadores->masterSet;

		if (select(fd_sets_entrenadores->maxFileDescriptorNumber + 1,
						&(fd_sets_entrenadores->readFileDescriptorSet),
						&(fd_sets_entrenadores->writeFileDescriptorSet),
						NULL, NULL) == -1){
			log_error(mapa_log,"Se ha producido un error al intentar atender las peticiones de los Entrenadores.");
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
	sem_post(&sem_listos);
	pthread_mutex_unlock(&mutex_entrenadores_listos);
	log_info(mapa_log, "Se agrega al Entrenador %s a la Cola de Listos", entrenador->nombre);
}

//Remover entrenador segun Round Robin
t_entrenador *remover_entrenador_listo_por_RR(){
	t_entrenador *entrenador = NULL;
	pthread_mutex_lock(&mutex_entrenadores_listos);
	//sem_wait(&sem_listos);
	entrenador = (t_entrenador *) list_remove(entrenadores_listos, 0);
	pthread_mutex_unlock(&mutex_entrenadores_listos);
	log_info(mapa_log, "Se remueve el Entrenador %s de la Cola de Listos.", entrenador->nombre);
	return entrenador;
}

//Remover entrenador segun Algoritmo Shortest Remaining Distance First
t_entrenador *remover_entrenador_listo_por_SRDF(){
	t_entrenador* entrenador = NULL;
	pthread_mutex_lock(&mutex_entrenadores_listos);
	//sem_wait(&sem_listos);
	srdf(entrenadores_listos);
	entrenador = (t_entrenador*)list_remove(entrenadores_listos,0);
	pthread_mutex_unlock(&mutex_entrenadores_listos);
	log_info(mapa_log, "Se remueve el Entrenador %s de la Cola de Listos.", entrenador->nombre);
	return entrenador;

}

//Agrega un entrenador a la cola bloqueados
void agregar_entrenador_a_bloqueados(t_entrenador *entrenador){
	pthread_mutex_lock(&mutex_cola_bloqueados);
	t_entrenadores_bloqueados *bloqueados = (t_entrenadores_bloqueados *)dictionary_get(entrenadores_bloqueados, entrenador->pokemon_bloqueado);
	queue_push(bloqueados->entrenadores, entrenador);
	sem_post(&sem_entrenadores_bloqueados[bloqueados->sem_index]); //TODO: VER porque no hace post
	pthread_mutex_unlock(&mutex_cola_bloqueados);
	log_info(mapa_log, "Se agrega al Entrenador %s a la Lista de Bloqueados.", entrenador->nombre);
}

/********* FUNCIONES DE INICIALIZACION *********/
void inicializar_estructuras(){

	//Estructuras
	entrenadores_listos = list_create();
	items = list_create();
	mensajes_entrenadores = list_create();

	datos_mapa = malloc(sizeof(t_datos_mapa));
	algoritmo=malloc(sizeof(char)*4);
	datos_mapa->items = items;
	datos_mapa->entrenador = NULL;

	//Semaforos
	inicializar_semaforo_mensajes();
	pthread_mutex_init(&mutex_desplaza_x, NULL);
	pthread_mutex_init(&mutex_desplaza_y, NULL);
	pthread_mutex_init(&mutex_entrenadores_listos, NULL);
	pthread_mutex_init(&mutex_cola_bloqueados, NULL);
	pthread_mutex_init(&mutex_recursos_pokenest,NULL);
	pthread_mutex_init(&mutex_pokenests,NULL);

	sem_init(&sem_listos, 0, 0);
	sem_init(&sem_mensajes, 0, 0);

	incializar_gestion_colas_bloqueados();

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
		case UBICACION_POKENEST:
			if(!recibir_mensaje_ubicacion_pokenest(mensajes_entrenadores, fd_entrenador, mapa_log)){
				log_info(mapa_log, "El Entrenador del FD %d se Desconecto, se procedera a liberar sus recursos.", fd_entrenador);
				//TODO: Agregar logica de Liberacion de Recursos
			} else {
				log_info(mapa_log, "Se recibio correctamente el mensaje UBICACION POKENES del Entrenador del FD %d.", fd_entrenador);
				sem_post(&sem_mensajes);
			}
			break;
		case AVANZAR_HACIA_POKENEST:
			log_info(mapa_log, "Enviaron el mensaje AVANZAR HACIA POKENEST del Entrenador del FD %d.", fd_entrenador);
			if(!recibir_mensaje_avanzar_hacia_pokenest(mensajes_entrenadores, fd_entrenador, mapa_log)){
				log_info(mapa_log, "El Entrenador del FD %d se Desconecto, se procedera a liberar sus recursos.", fd_entrenador);
				//TODO: Agregar logica de Liberacion de Recursos
			} else {
				log_info(mapa_log, "Se recibio correctamente el mensaje AVANZAR HACIA POKENEST del Entrenador del FD %d.", fd_entrenador);
				sem_post(&sem_mensajes);
			}
			break;
		case ATRAPAR_POKEMON:
			log_info(mapa_log, "Enviaron el mensaje ATRAPAR POKEMON del Entrenador del FD %d.", fd_entrenador);
			if(!recibir_mensaje_atrapar_pokemon(mensajes_entrenadores, fd_entrenador, mapa_log)){
				log_info(mapa_log, "El Entrenador del FD %d se Desconecto, se procedera a liberar sus recursos.", fd_entrenador);
				//TODO: Agregar logica de Liberacion de Recursos
			} else {
				log_info(mapa_log, "Se recibio correctamente el mensaje ATRAPAR POKEMON del Entrenador del FD %d.", fd_entrenador);
				sem_post(&sem_mensajes);
			}
			break;
		case OBJETIVO_CUMPLIDO:
			log_info(mapa_log, "Enviaron el mensaje OBJETIVO CUMPLIDO del Entrenador del FD %d.", fd_entrenador);
			if(!recibir_mensaje_objetivo_cumplido(mensajes_entrenadores, fd_entrenador, mapa_log)){
				log_info(mapa_log, "El Entrenador del FD %d se Desconecto, se procedera a liberar sus recursos.", fd_entrenador);
				//TODO: Agregar logica de Liberacion de Recursos
			} else {
				log_info(mapa_log, "Se recibio correctamente el mensaje OBJETIVO CUMPLIDO del Entrenador del FD %d.", fd_entrenador);
				sem_post(&sem_mensajes);
			}
			break;
		default:
			log_error(mapa_log, "Se ha producido un error al intentar atender a la peticion del Entrenador");
			break;
		}
}

void recibir_nuevo_entrenador(int fd){
	log_info(mapa_log, "Se van a tomar los datos para inscribir al nuevo Entrenador.");
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
	entrenador->id = entrenador_id + 1;
	entrenador->fd = fd;
	entrenador->nombre = nombre;
	entrenador->caracter =  *caracter;
	entrenador->posicion->x = POSICION_INICIAL_X;
	entrenador->posicion->y = POSICION_INICIAL_Y;
	entrenador->pokemons = list_create();

	log_info(mapa_log,"Bienvenido Entrenador %s N° %d.", entrenador->nombre, fd);

	datos_mapa->entrenador = entrenador;

	//Creamos la estructura donde van a ir los mensajes
	inicializar_mensajes_entrenador(mensajes_entrenadores, fd, mapa_log);

	agregar_entrenador_a_listos(entrenador);

	/* Muestro en el Mapa al Entrenador */
	ingreso_nuevo_entrenador(items, entrenador, nombre_mapa);
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

void entregar_pokemon(t_entrenador* entrenador, t_pokemon *pokemon, char pokenest_id){
	int tamanio_nombre_archivo = string_length(pokemon->nombre_archivo);
	list_add(entrenador->pokemons, pokemon);

	//Envio el nombre del archivo al Entrenador
	enviarInt(entrenador->fd, POKEMON_CONCEDIDO);
	enviarInt(entrenador->fd, (sizeof(char) * tamanio_nombre_archivo));
	if(enviarMensaje(entrenador->fd, pokemon->nombre_archivo) < 0){
		//Consideramos que el Entrenador se Deconecto - Hay que liberar recursos
		liberar_recursos_entrenador(entrenador);
	} else {
		//Resto el Recurso de la Interfaz de Mapa
		disminuir_recursos_de_pokenest(items, pokenest_id, nombre_mapa);
		entrenador->pokemon_bloqueado = NULL; //Limpio el ID del Pokemon porque ya se otorgo
	}
}

void entregar_medalla(t_entrenador *entrenador, char* nombre_mapa){
	log_info(mapa_log, "Se va a Entregar la Medalla a %s.", entrenador->nombre);
	char *ruta_medalla = get_medalla_path(ruta_pokedex, nombre_mapa);
	log_info(mapa_log, "La ruta de la Medalla es %s.", ruta_medalla);
	enviarInt(entrenador->fd, strlen(ruta_medalla));
	enviarMensaje(entrenador->fd, ruta_medalla);

	//Libero Entrenador
	liberar_recursos_entrenador(entrenador);
}

void enviar_posicion_pokenest(int fd , t_mensajes *mensajes){
	char *nombre_pokenest = (char *)obtener_mensaje(mensajes);
	pthread_mutex_lock(&mutex_pokenests);
	t_pokenest *pokenest = get_pokenest_by_identificador(lista_pokenests, *nombre_pokenest);
	pthread_mutex_unlock(&mutex_pokenests);
	log_info(mapa_log, "Se envia posicion del Pokenest: %s", pokenest->nombre);
	enviarInt(fd,pokenest->posicion->x);
	enviarInt(fd,pokenest->posicion->y);
}

void avanzar_hacia_pokenest(t_entrenador *entrenador, t_mensajes *mensajes){
	entrenador->posicion->x = (int) obtener_mensaje(mensajes);
	mover_entrenador_en_mapa(items, entrenador, nombre_mapa);
	entrenador->posicion->y = (int) obtener_mensaje(mensajes);
	mover_entrenador_en_mapa(items, entrenador, nombre_mapa);
	log_info(mapa_log, "El Entrenador %s se movio a la posicion (%d, %d).", entrenador->nombre, entrenador->posicion->x, entrenador->posicion->y);
}

void solicitar_atrapar_pokemon(t_entrenador *entrenador, t_mensajes *mensajes){
	char *nombre_pokemon = (char *)obtener_mensaje(mensajes);
	entrenador->pokemon_bloqueado = nombre_pokemon;
	log_info(mapa_log, "El Entrenador %s solicito atrapar a un Pokemon.", entrenador->nombre);
}

/********* FUNCION ENCARGADA DEL MANEJO DE LAS SYSTEM CALLS*********/
void system_call_catch(int signal){

	if (signal == SIGUSR2){
		log_info(mapa_log, "Se ha enviado la señal SIGUSR2. Se actualizarán las variables de Configuración.");
		set_algoritmoActual();
		set_quantum();
		set_retardo();
		set_interbloqueo();
	}
}

/********* FUNCIONES ENCARGADAS DE LA PLANIFICACION DE LOS ENTRENADORES *********/
void administrar_turnos() {
	t_entrenador* entrenador;
	char *round_robin = string_new();
	string_append(&round_robin, "RR");
	bool es_algoritmo_rr = FALSE;

	while (1){
		sem_wait(&sem_listos);

		//Si es verdadero, se extrae por algoritmo de Round Robin
		if(string_equals_ignore_case(algoritmo_actual(), round_robin)){
			entrenador = remover_entrenador_listo_por_RR();
			es_algoritmo_rr = TRUE;
		} else {
			entrenador = remover_entrenador_listo_por_SRDF();
			es_algoritmo_rr = FALSE;;
		}

		atender_Viaje_Entrenador(entrenador, es_algoritmo_rr);
	}
}

void atender_Viaje_Entrenador(t_entrenador* entrenador, bool es_algoritmo_rr){
	int turnos = 0;
	bool bloqueado = FALSE; //Flag utilizado para saber si un Entrenador se Bloqueo
	bool finalizo = FALSE; //Falg utilizado para saber si el Entrenador Finalizo
	int instruccion;

	while ((!es_algoritmo_rr || turnos < mapa_quantum) && !bloqueado && !finalizo){
		sem_wait(&sem_mensajes);

		log_info(mapa_log, "Al Entrenador %s le toca el turno %d", entrenador->nombre, turnos);

		t_mensajes *mensajes_entrenador = obtener_mensajes_de_entrenador(mensajes_entrenadores, entrenador->fd);
		instruccion = (int) obtener_mensaje(mensajes_entrenador);

		switch(instruccion){
			case UBICACION_POKENEST:
				enviar_posicion_pokenest(entrenador->fd, mensajes_entrenador);
				break;
			case AVANZAR_HACIA_POKENEST:
				avanzar_hacia_pokenest(entrenador, mensajes_entrenador);
				break;
			case ATRAPAR_POKEMON:
				solicitar_atrapar_pokemon(entrenador, mensajes_entrenador);
				bloqueado = TRUE;
				break;
			case OBJETIVO_CUMPLIDO:
				entregar_medalla(entrenador, nombre_mapa);
				finalizo = TRUE;
				break;
			default:
				log_error(mapa_log, "Se ha producido un error al tratar de atender instruccion del Entrenador.");
				break;
		}

		turnos++;

		sleep(retardo);
	}

	//Finalizo el Turno del Entrenador
	if(!bloqueado){
		if(!finalizo){
			agregar_entrenador_a_listos(entrenador);
		}
	} else {
		log_info(mapa_log, "Se va a Bloquear al Entrenador %s.", entrenador->nombre);
		agregar_entrenador_a_bloqueados(entrenador);
	}
}

char* algoritmo_actual() {
	return algoritmo;
}

int quantum_actual() {
	return mapa_quantum;
}

bool menor_distancia(t_entrenador* unEntrenador, t_entrenador* otroEntrenador){
	int unaDistancia = distancia_a_pokenest(unEntrenador);
	int otraDistancia = distancia_a_pokenest(otroEntrenador);
	return (unaDistancia < otraDistancia);
}

double distancia_a_pokenest(t_entrenador* entrenador){
	int deltaX = entrenador->pokenx - entrenador->posicion->x;
	int deltaY = entrenador->pokeny - entrenador->posicion->y;

	return sqrt(deltaX*deltaX + deltaY*deltaY);
}

void srdf(t_list* entrenadores_listos){
	bool (*pf)(t_entrenador*,t_entrenador*) = menor_distancia;
	list_sort(entrenadores_listos, pf);
}

/* Actualizacion de valores desde el Archivo de comfiguracion */
void set_algoritmoActual(){
	algoritmo = get_mapa_algoritmo(metadata);
}
void set_quantum(){
	mapa_quantum = get_mapa_quantum(metadata);
}

void set_retardo(){
	retardo = get_mapa_retardo(metadata);
}
void set_interbloqueo(){
	interbloqueo = get_mapa_tiempo_deadlock(metadata);
}


void administrar_bloqueados(char *pokenest_id){
	//Obtengo la Estructura de Cola de Entrenadores Bloqueados
	pthread_mutex_lock(&mutex_cola_bloqueados);
	t_entrenadores_bloqueados *bloqueados = (t_entrenadores_bloqueados *) dictionary_get(entrenadores_bloqueados, pokenest_id);
	pthread_mutex_unlock(&mutex_cola_bloqueados);

	t_queue *entrenadores = bloqueados->entrenadores;

	while(1){
		sem_wait(&sem_pokemones_disponibles[bloqueados->sem_index]);
		sem_wait(&sem_entrenadores_bloqueados[bloqueados->sem_index]);

		log_info(mapa_log, "Hola, entre a ejecutar en el Hilo de Bloqueados %d.", bloqueados->sem_index);

		//Obtengo el Entrenador Bloqueado
		pthread_mutex_lock(&mutex_cola_bloqueados);
		t_entrenador *entrenador = queue_pop(entrenadores);
		pthread_mutex_unlock(&mutex_cola_bloqueados);
		log_info(mapa_log, "Se removio al Entrenador %s a la Cola de Bloqueados.", entrenador->nombre);

		//Obtengo el Pokemon
		pthread_mutex_lock(&mutex_pokenests);
		t_pokemon *pokemon = get_pokemon_by_identificador(lista_pokenests, *pokenest_id);
		pthread_mutex_unlock(&mutex_pokenests);

		//Entregamos el Pokemon al Entrenador
		entregar_pokemon(entrenador, pokemon, *pokenest_id);

		log_info(mapa_log, "Se otorgo el Pokemon %s al Entrenador %s.", pokemon->nombre, entrenador->nombre);

		agregar_entrenador_a_listos(entrenador);
	}
}

void incializar_gestion_colas_bloqueados(){
	entrenadores_bloqueados = dictionary_create();
	int cantidad_pokenest = list_size(lista_pokenests);
	pthread_t *bloqueados_threads_vec = malloc(cantidad_pokenest * sizeof(pthread_t));
	sem_entrenadores_bloqueados = malloc(cantidad_pokenest * sizeof(sem_t));
	sem_pokemones_disponibles = malloc(cantidad_pokenest * sizeof(sem_t));
	int i;
	t_pokenest *pokenest;
	char *pokenest_id;

	//Contruyo el Diccionario de las Colas de Bloquados, incializo los Semaforos y creo los Hilos de Administracion
	for(i = 0; i < cantidad_pokenest; i++){
		t_entrenadores_bloqueados *cola_bloqueados = malloc(sizeof(t_entrenadores_bloqueados));
		pokenest = list_get(lista_pokenests, i);
		sem_init(&(sem_entrenadores_bloqueados[i]), 0, 0); //Inicializo Semaforo de Entrenadores
		sem_init(&(sem_pokemones_disponibles[i]), 0, pokenest->cantPokemons); //Inicializo Semaforo de Pokemons
		cola_bloqueados->sem_index = i;
		cola_bloqueados->entrenadores = queue_create();
		pokenest_id = string_repeat(pokenest->caracter, 1);
		pthread_mutex_lock(&mutex_cola_bloqueados);
		dictionary_put(entrenadores_bloqueados, pokenest_id, cola_bloqueados);
		pthread_mutex_unlock(&mutex_cola_bloqueados);

		//Creo los Hilos Administradores de Bloquados
		pthread_create(&(bloqueados_threads_vec[i]), NULL, (void *)administrar_bloqueados, (void *) pokenest_id);
	}
}

void liberar_recursos_entrenador(t_entrenador *entrenador){
	int cant_pokemons = list_size(entrenador->pokemons);
	int i, fd;

	log_info(mapa_log, "Voy a liberar los recursos del Entrenador");

	pthread_mutex_lock(&mutex_pokenests);
	t_pokemon *pokemon;
	for(i = 0; i < cant_pokemons; i++){
		pokemon = list_remove(entrenador->pokemons, 0); //Siempre saco el Pokemon que quedo en la posicion 0
		//Agrego el Pokemon al Pokenest
		add_pokemon_pokenest(lista_pokenests, pokemon);
		//Actualizo la interfaz Grafica sumando un recurso al Pokenest
		aumentar_recursos_de_pokenest(items, pokemon->pokenest_id, nombre_mapa);
	}
	pthread_mutex_unlock(&mutex_pokenests);

	//Elimino el entrenador de la Interfaz grafica
	eliminar_entrenador(items, entrenador->caracter, nombre_mapa);

	fd = entrenador->fd;

	//Elimino los Mensajes
	eliminar_cola_mensajes_entrenador(mensajes_entrenadores, fd, mapa_log);

	//Libero Memoria
	list_destroy(entrenador->pokemons);
	free(entrenador->posicion);
	log_info(mapa_log, "El Entrenador %s a abandonado el Mapa.", entrenador->nombre);
	free(entrenador);
	close(fd);
}

/* Funciones para la Gestion de Entrenadores Interbloqueados */
void add_entrenadores_interbloqueados(char *key, void *entrenadores){
	t_list *entrenadores_lista = (t_list *) entrenadores;
	int cant_entrenadores = list_size(entrenadores_lista);
	int cant_pokenets = list_size(lista_pokenests);
	int i, j, cant_pokemons, pokenest_index;

	for(i = 0; i < cant_entrenadores; i++){
		t_entrenador_interbloqueado *interbloqueado = malloc(sizeof(t_entrenador_interbloqueado));
		interbloqueado->entrenador = malloc(sizeof(t_entrenador));
		interbloqueado->entrenador = (t_entrenador *) list_get(entrenadores, i);
		interbloqueado->marcado = 0;
		interbloqueado->solicitud = malloc(cant_pokenets * sizeof(int));
		interbloqueado->asignacion = malloc(cant_pokenets * sizeof(int));
		cant_pokemons = list_size(interbloqueado->entrenador->pokemons);

		//Inicializo los Arrays
		for(j = 0; j < cant_pokenets; j++){
			interbloqueado->asignacion[j] = 0;
			interbloqueado->solicitud[j] = 0;
		}

		//Armo Array de Asignaciones
		for(j = 0; j < cant_pokemons; j++){
			t_pokemon *pokemon = (t_pokemon *) list_get(interbloqueado->entrenador->pokemons, j);
			pthread_mutex_lock(&mutex_pokenests);
			pokenest_index = get_pokenest_index_by_pokemon_id(lista_pokenests, pokemon->pokenest_id);
			pthread_mutex_unlock(&mutex_pokenests);
			interbloqueado->asignacion[pokenest_index] = interbloqueado->asignacion[pokenest_index] + 1;
		}

		//Actualizo el Array de Solicitudes
		pthread_mutex_lock(&mutex_pokenests);
		pokenest_index = get_pokenest_index_by_pokemon_id(lista_pokenests, *interbloqueado->entrenador->pokemon_bloqueado);
		pthread_mutex_unlock(&mutex_pokenests);
		interbloqueado->solicitud[pokenest_index] = interbloqueado->solicitud[pokenest_index] + 1;

		list_add(lista_interbloqueo, interbloqueado);
	}
}

void chequear_interbloqueados(){
	//sleep(interbloqueo);

	int cant_pokenets = list_size(lista_pokenests);
	int i, cant_entrenadores;
	int *disponibles = malloc(cant_pokenets * sizeof(int));
	lista_interbloqueo = list_create();
	void (*add_entrenadores_interbloqueados) (char*, void*) = add_entrenadores_interbloqueados;
	t_list *entrenadores_interbloqueados = list_create();

	//Preparo el Array de Disponobiles
	pthread_mutex_lock(&mutex_pokenests);
	for(i = 0; i < cant_pokenets; i++){
		t_pokenest *pokenets = (t_pokenest *) list_get(lista_pokenests, i);
		disponibles[i] = pokenets->cantPokemons;
	}
	pthread_mutex_unlock(&mutex_pokenests);

	//Preparo la estructura de Entrenadores
	pthread_mutex_lock(&mutex_cola_bloqueados);
	dictionary_iterator(entrenadores_bloqueados, add_entrenadores_interbloqueados);
	pthread_mutex_unlock(&mutex_cola_bloqueados);

	cant_entrenadores = list_size(lista_interbloqueo);

	//Marcos los entrenadores que no tienen recursos asignados
	for(i = 0; i < cant_entrenadores; i++){
		t_entrenador_interbloqueado *entrenador = (t_entrenador_interbloqueado *) list_get(lista_interbloqueo, i);

		if(chequear_pokemones_sin_asignar(cant_pokenets, entrenador->asignacion)){
			entrenador->marcado = 1; //Marco al Entrenador
		}
	}

	//Busco los Entrenadores que no fueron marcados usando su matris de solicitud
	int termino_ciclo = 0;

	while(!termino_ciclo){
		if(!recorrer_solicitudes(lista_interbloqueo, cant_pokenets, cant_entrenadores, disponibles)){
			termino_ciclo = 1;
		}
	}

	//Libero la lista de Chequeo de Interbloqueo
	for(i = 0; i < cant_entrenadores; i++){
		t_entrenador_interbloqueado *entrenador = (t_entrenador_interbloqueado *) list_remove(lista_interbloqueo, i);
		if(!entrenador->marcado){
			list_add(entrenadores_interbloqueados, entrenador->entrenador);
		}

		free(entrenador->asignacion);
		free(entrenador->entrenador);
		free(entrenador->solicitud);
	}

	list_destroy(lista_interbloqueo);

	//Informar los entrenadores que estan interbloqueados por log
}
