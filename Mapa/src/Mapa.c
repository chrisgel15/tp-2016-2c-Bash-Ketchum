
#include "Mapa.h"

//Mutexs de Estructura de Estados
pthread_mutex_t mutex_entrenadores_listos;
pthread_mutex_t mutex_cola_bloqueados;
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
t_list *mensajes_entrenadores;
t_list *bloqueados; //Esta estrucutra se va a utilizar para buscar entre todos los Entrenadores Bloquedos cuando alguno cierre su conexion

//Lista de nombres de pokenests
//char **pokenests;
t_list *lista_pokenests;

//Lista de items a dibujar en el Mapa
t_list *items;

//Log
t_log *mapa_log;

//metadata
t_config *metadata;

//Algoritmo
char *algoritmo;

//Quantum
int mapa_quantum;

//Retardo
int64_t retardo;

//Interbloqueo
int64_t interbloqueo;
t_list *lista_interbloqueo;

//Nombre de Mapa
char *nombre_mapa;
char *ruta_pokedex;
char *log_nombre;
char *pokenest_dir;

//Ids de Entrenadores
int entrenador_id = 0;

//Hilos
pthread_t p_planificador;
pthread_t p_interbloqueo;
pthread_t *bloqueados_threads_vec;

//****************************************************
// Prototipos de funciones internas

int menor_distancia(t_entrenador*, t_entrenador*);
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
	log_nombre = string_new(); //Creo el nombre del Archivo de Log
	string_append(&log_nombre, nombre_mapa);
	string_append(&log_nombre, ".log");
	mapa_log = CreacionLogWithLevel(log_nombre, programa_nombre, log_level);
	mapa_log->is_active_console = false;
	log_info(mapa_log, "Se ha creado el Log para el Mapa.");

	//Inicializamos el Listado de los Pokenests con sus respectivos Pokemons
	//lista_pokenests = get_listado_pokenest(argv[2] , nombre_mapa);
	pokenest_dir = get_pokenest_path_dir(ruta_pokedex , nombre_mapa);
	lista_pokenests = get_listado_pokenest(ruta_pokedex , nombre_mapa, pokenest_dir);

	//Inicializamos Estructuras
	inicializar_estructuras();

	//Manejo de System Calls
	//signal(SIGUSR2, system_call_catch);

	//Creamos el Servidor de Entrenadores
	int puerto_entrenadores = get_mapa_puerto(metadata);
	int listener_entrenadores;
	ltn_sock_addinfo* ltn_entrenadores;
	char *puerto_escucha = string_itoa(puerto_entrenadores);
	ltn_entrenadores = createSocket(puerto_escucha);
	free(puerto_escucha);
	listener_entrenadores = doBind(ltn_entrenadores);
	listener_entrenadores = doListen(listener_entrenadores, 100);

	ltn_fd_sets *fd_sets_entrenadores = createFdSets(listener_entrenadores);

	log_info(mapa_log, "A la espera de Entrenadores Pokémon.");

	//Algoritmo
	algoritmo = get_mapa_algoritmo(metadata);

	//Quantum
	mapa_quantum = get_mapa_quantum(metadata);

	//Retardo
	retardo = get_mapa_retardo(metadata) * 1000;

	//Interbloqueo
	interbloqueo = get_mapa_tiempo_deadlock(metadata) * 1000;

	//Inicializamos la Interfaz Grafica
	inicializar_mapa(items, lista_pokenests, nombre_mapa);

	//Hilo Planificador
	//pthread_t planificador;
	pthread_create(&p_planificador, NULL, (void *) administrar_turnos, NULL);

	//Hilo Interbloqueo
	//pthread_t interbloqueo;
	pthread_create(&p_interbloqueo, NULL, (void *) chequear_interbloqueados, NULL);

	//Hilo Interbloqueo
	pthread_t p_calls_manager;
	pthread_create(&p_calls_manager, NULL, (void *) system_call_manager, NULL);

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
	free(pokenest_dir);
	return EXIT_SUCCESS;
}

/********* FUNCIONES PARA EL MANEJO DE ESTRUCTURAS DE ESTADOS *********/

//Agrega un Nuevo Entrenador a la Cola de Listos
void agregar_entrenador_a_listos(t_entrenador *entrenador) {
	pthread_mutex_lock(&mutex_entrenadores_listos);
	entrenador->timpo_ingreso_listo = time(0);
	list_add(entrenadores_listos, entrenador);
	int listos_size = list_size(entrenadores_listos);
	char *entrenadores = string_new();
	int i;
	for(i = 0; i < listos_size; i++){
		t_entrenador *entrenador_listo = (t_entrenador *) list_get(entrenadores_listos, i);
		string_append(&entrenadores, " ");
		string_append(&entrenadores, entrenador_listo->nombre);
	}

	sem_post(&sem_listos);
	pthread_mutex_unlock(&mutex_entrenadores_listos);
	log_info(mapa_log, "Se agrega al Entrenador %s a la Cola de Listos. Entrenadores Listos:%s", entrenador->nombre, entrenadores);
	free(entrenadores);
}

//Remover Entrenador segun Round Robin
t_entrenador *remover_entrenador_listo_por_RR(){
	t_entrenador *entrenador = NULL;
	pthread_mutex_lock(&mutex_entrenadores_listos);
	//sem_wait(&sem_listos);
	//entrenador = (t_entrenador *) list_remove(entrenadores_listos, 0);
	entrenador = remover_entrenador_mas_viejo();
	pthread_mutex_unlock(&mutex_entrenadores_listos);
	log_info(mapa_log, "Se remueve el Entrenador %s de la Cola de Listos.", entrenador->nombre);
	return entrenador;
}

t_entrenador *remover_entrenador_mas_viejo(){
	int entrenadores_listos_size = list_size(entrenadores_listos);
	int i = 0, j = 0;
	t_entrenador *entrenador = NULL, *entrenador_compare = NULL;
	bool mas_viejo = true, encontrado = false;
	log_info(mapa_log, "Voy a verificar cual es el Entrenador mas Viejo.");
	while((i  < entrenadores_listos_size) && !encontrado){
		entrenador = (t_entrenador *) list_get(entrenadores_listos, i);
		j = i + 1;
		while((j < entrenadores_listos_size) && mas_viejo){
			entrenador_compare = (t_entrenador *) list_get(entrenadores_listos, j);
			log_info(mapa_log, "Comparo a %s con %s.", entrenador->nombre, entrenador_compare->nombre);

			if(entrenador->timpo_ingreso_listo > entrenador_compare->timpo_ingreso_listo){
				mas_viejo = false;
			}
			j = j + 1;
		}

		if(!mas_viejo){
			i = i + 1;
			mas_viejo = true;
		} else {
			encontrado = true;
		}
	}

	entrenador = (t_entrenador *) list_remove(entrenadores_listos, i);
	log_info(mapa_log, "El Entrenador mas viejos es %s.", entrenador->nombre);
	return entrenador;
}

//Remover Entrenador segun Algoritmo Shortest Remaining Distance First
t_entrenador *remover_entrenador_listo_por_SRDF(){ //20161214 - FM
	t_entrenador* entrenador = NULL;
	pthread_mutex_lock(&mutex_entrenadores_listos);
	//sem_wait(&sem_listos);
	int cant_listos = list_size(entrenadores_listos);
	bool sin_ubicacion = false;
	int i = 0;
	int aux_index;
	while(i < cant_listos && !sin_ubicacion){
		entrenador = list_get(entrenadores_listos, i);

		if(!entrenador->conoce_ubicacion){
			aux_index = i;
			sin_ubicacion = true;

		}
		i++;
	}
	if(!sin_ubicacion){
		srdf(entrenadores_listos);
		entrenador = (t_entrenador*)list_remove(entrenadores_listos,0);
	}
	else
		entrenador = (t_entrenador*)list_remove(entrenadores_listos,aux_index);

	pthread_mutex_unlock(&mutex_entrenadores_listos);
	log_info(mapa_log, "Se remueve el Entrenador %s de la Cola de Listos.", entrenador->nombre);
	return entrenador;
}

//Agrega un entrenador a la cola bloqueados
void agregar_entrenador_a_bloqueados(t_entrenador *entrenador){
	pthread_mutex_lock(&mutex_cola_bloqueados);
	t_entrenadores_bloqueados *cola_bloqueados = (t_entrenadores_bloqueados *)dictionary_get(entrenadores_bloqueados, entrenador->pokemon_bloqueado);
	queue_push(cola_bloqueados->entrenadores, entrenador);
	t_list *entrenadores_bloqueados_lista = cola_bloqueados->entrenadores->elements;
	int bloqueados_size = list_size(entrenadores_bloqueados_lista);
	char *entrenadores = string_new();
	int i = 0;
	for(i = 0; i < bloqueados_size; i++){
		t_entrenador *entrenador_bloqueado = (t_entrenador *) list_get(entrenadores_bloqueados_lista, i);
		string_append(&entrenadores, " ");
		string_append(&entrenadores, entrenador_bloqueado->nombre);
	}

	sem_post(&sem_entrenadores_bloqueados[cola_bloqueados->sem_index]);
	pthread_mutex_unlock(&mutex_cola_bloqueados);
	log_info(mapa_log, "Se agrega al Entrenador %s a la Cola de Bloqueados del Pokenest %s. Entrenadores Bloqueados:%s", entrenador->nombre, entrenador->pokemon_bloqueado, entrenadores);
	free(entrenadores);
}

void sumar_recurso_pokemon(t_pokemon_mapa *pokemon){
	char *pokenest_id = string_repeat(pokemon->pokenest_id, 1);
	//Sumo el el recurso al Pokenest
	add_pokemon_pokenest(lista_pokenests, pokemon);
	t_entrenadores_bloqueados *bloqueados_4 = (t_entrenadores_bloqueados *)dictionary_get(entrenadores_bloqueados, pokenest_id);
	//Aumento el Semaforo
	sem_post(&sem_pokemones_disponibles[bloqueados_4->sem_index]);
	free(pokenest_id);
}

t_entrenador *remover_entrenador_de_bloqueados(t_entrenador *entrenador){
	//pthread_mutex_lock(&mutex_cola_bloqueados);
	t_entrenadores_bloqueados *bloqueados_3 = (t_entrenadores_bloqueados *)dictionary_get(entrenadores_bloqueados, entrenador->pokemon_bloqueado);
	t_list *entrenadores = bloqueados_3->entrenadores->elements;
	int i = 0, entrenadores_size = list_size(entrenadores);
	t_entrenador *entrenador_bloqueado = NULL;
	int index_encontrado;
	for(i = 0; i < entrenadores_size; i++){
		entrenador_bloqueado = list_get(entrenadores, i);
		if(entrenador_bloqueado->id == entrenador->id){
			index_encontrado = i;
		}
	}

	entrenador = (t_entrenador *)list_remove(entrenadores, index_encontrado);
	//int bloqueados_size = queue_size(bloqueados->entrenadores);
	sem_wait(&sem_entrenadores_bloqueados[bloqueados_3->sem_index]); //Decremento el Semaforo

	//pthread_mutex_unlock(&mutex_cola_bloqueados);
	log_info(mapa_log, "El Entrenador %s fue eliminado de la Lista de Bloqueados.", entrenador->nombre);
	return entrenador;
}

/********* FUNCIONES DE INICIALIZACION *********/
void inicializar_estructuras(){

	//Estructuras
	entrenadores_listos = list_create();
	items = list_create();
	mensajes_entrenadores = list_create();

	algoritmo = malloc((sizeof(char)) * 4);

	//Semaforos
	inicializar_semaforo_mensajes();
	pthread_mutex_init(&mutex_entrenadores_listos, NULL);
	pthread_mutex_init(&mutex_cola_bloqueados, NULL);
	pthread_mutex_init(&mutex_pokenests,NULL);

	sem_init(&sem_listos, 0, 0);
	//sem_init(&sem_mensajes, 0, 0);

	incializar_gestion_colas_bloqueados();

	log_info(mapa_log, "Se inicializaron las Estructuras y los Semaforos.");
}

/********* FUNCIONES PARA RECIBIR PETICIONES DE LOS ENTRENADORES *********/
void atender_entrenador(int fd_entrenador, int codigo_instruccion){
	log_info(mapa_log, "Voy a atender al Entrenador con Fd %d - Codigo: %d.", fd_entrenador, codigo_instruccion);
	switch(codigo_instruccion){
		case SOY_ENTRENADOR:
			recibir_nuevo_entrenador(fd_entrenador);
			break;
		case UBICACION_POKENEST:
			if(!recibir_mensaje_ubicacion_pokenest(mensajes_entrenadores, fd_entrenador, mapa_log)){
				log_info(mapa_log, "El Entrenador del FD %d se Desconecto, se procedera a liberar sus recursos.", fd_entrenador);
				//TODO: Agregar logica de Liberacion de Recursos
			} else {
				log_info(mapa_log, "Se recibio correctamente el mensaje UBICACION POKENES del Entrenador del FD %d.", fd_entrenador);
				//sem_post(&sem_mensajes);
			}
			break;
		case AVANZAR_HACIA_POKENEST:
			log_info(mapa_log, "Enviaron el mensaje AVANZAR HACIA POKENEST del Entrenador del FD %d.", fd_entrenador);
			if(!recibir_mensaje_avanzar_hacia_pokenest(mensajes_entrenadores, fd_entrenador, mapa_log)){
				log_info(mapa_log, "El Entrenador del FD %d se Desconecto, se procedera a liberar sus recursos.", fd_entrenador);
				//TODO: Agregar logica de Liberacion de Recursos
			} else {
				log_info(mapa_log, "Se recibio correctamente el mensaje AVANZAR HACIA POKENEST del Entrenador del FD %d.", fd_entrenador);
				//sem_post(&sem_mensajes);
			}
			break;
		case ATRAPAR_POKEMON:
			log_info(mapa_log, "Enviaron el mensaje ATRAPAR POKEMON del Entrenador del FD %d.", fd_entrenador);
			if(!recibir_mensaje_atrapar_pokemon(mensajes_entrenadores, fd_entrenador, mapa_log)){
				log_info(mapa_log, "El Entrenador del FD %d se Desconecto, se procedera a liberar sus recursos.", fd_entrenador);
				//TODO: Agregar logica de Liberacion de Recursos
			} else {
				log_info(mapa_log, "Se recibio correctamente el mensaje ATRAPAR POKEMON del Entrenador del FD %d.", fd_entrenador);
				//sem_post(&sem_mensajes);
			}
			break;
		case OBJETIVO_CUMPLIDO:
			log_info(mapa_log, "Enviaron el mensaje OBJETIVO CUMPLIDO del Entrenador del FD %d.", fd_entrenador);
			if(!recibir_mensaje_objetivo_cumplido(mensajes_entrenadores, fd_entrenador, mapa_log)){
				log_info(mapa_log, "El Entrenador del FD %d se Desconecto, se procedera a liberar sus recursos.", fd_entrenador);
			} else {
				log_info(mapa_log, "Se recibio correctamente el mensaje OBJETIVO CUMPLIDO del Entrenador del FD %d.", fd_entrenador);
				//sem_post(&sem_mensajes);
			}
			break;
		default:
			log_error(mapa_log, "Se ha producido un error al intentar atender a la peticion del Entrenador N° %d. Código recibido: %d", fd_entrenador, codigo_instruccion);
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
	int tamanio_texto = 0;

	//Recibo el nombre
	tamanio_texto = recibirInt(fd, result, mapa_log);
	nombre = malloc(sizeof(char) * (tamanio_texto + 1));
	recibirMensaje(fd, nombre, tamanio_texto, mapa_log);
	tamanio_texto = 0;
	//Recibo el Caracter
	tamanio_texto = recibirInt(fd, result, mapa_log);
	caracter = malloc(sizeof(char) * (tamanio_texto + 1));
	recibirMensaje(fd, caracter, tamanio_texto, mapa_log);

	//Cargo la estructura del Entrenador con los datos recibidos por Socket
	//La posicion inicial es (0;0)
	entrenador_id = entrenador_id + 1; //Incremento el Id
	entrenador->id = entrenador_id;
	entrenador->fd = fd;
	entrenador->nombre = nombre;
	//entrenador->caracter =  *caracter;
	entrenador->caracter = caracter[0];
	entrenador->posicion->x = POSICION_INICIAL_X;
	entrenador->posicion->y = POSICION_INICIAL_Y;
	entrenador->pokemons = list_create();
	entrenador->tiempo_ingreso = time(0); //Guardo la Fecha y Hora de Ingreso del Entrenador al Mapa
	entrenador->pokemon_bloqueado = string_new();
	entrenador -> conoce_ubicacion = false;

	free(result);
	free(caracter);

	log_info(mapa_log,"Bienvenido Entrenador %s N° %d.", entrenador->nombre, fd);

	//Creamos la estructura donde van a ir los mensajes
	inicializar_mensajes_entrenador(mensajes_entrenadores, fd, mapa_log);

	agregar_entrenador_a_listos(entrenador);

	//Muestro en el Mapa al Entrenador
	ingreso_nuevo_entrenador(items, entrenador, nombre_mapa);
}

void despedir_entrenador(int fd_entrenador){
	log_info(mapa_log, "Se despide el Entrenador N° %d del Mapa.", fd_entrenador);

	pthread_mutex_lock(&mutex_entrenadores_listos);
	pthread_mutex_lock(&mutex_cola_bloqueados);

	t_entrenador *despedido = NULL;
	int i = 0, index_listado = 0;

	//Busco al entrenador en la lista de Entrenadores Listos
	int listos_size = list_size(entrenadores_listos);
	for(i = 0; i < listos_size; i++){
		t_entrenador *listo = (t_entrenador *) list_get(entrenadores_listos, i);
		if(listo->fd == fd_entrenador){
			despedido = listo;
			index_listado = i;
		}
	}

	//Si no encontre al Entrenador busco en los Entrenadores Bloqueados
	if(despedido == NULL){
		bloqueados = list_create();
		void (*add_entrenadores_bloqueados_iterator) (char*, void*) = add_entrenadores_bloqueados;
		dictionary_iterator(entrenadores_bloqueados, add_entrenadores_bloqueados_iterator);
		int bloqueados_size = list_size(bloqueados);
		for(i = 0; i < bloqueados_size; i++){
			t_entrenador *bloqueado = (t_entrenador *) list_get(bloqueados, i);
			if(bloqueado->fd == fd_entrenador){
				despedido = bloqueado;
			}
		}
		list_destroy(bloqueados);

		if(despedido != NULL){
			despedido = remover_entrenador_de_bloqueados(despedido);
		}

	} else {
		despedido = (t_entrenador *) list_remove(entrenadores_listos, index_listado);
	}

	//Si encontre al Entrenador, libero los Recursos
	if(despedido != NULL){
		liberar_recursos_entrenador(despedido, 0);
	} else {
		log_info(mapa_log, "No se encontro el Entrenador para Despedir. Se procedera a Enviarle un Mensaje.");
		entrenador_desconectado(mensajes_entrenadores, fd_entrenador);
		sem_post(&sem_mensajes);
	}

	pthread_mutex_unlock(&mutex_entrenadores_listos);
	pthread_mutex_unlock(&mutex_cola_bloqueados);
}

void add_entrenadores_bloqueados(char *key, void *entrenadores_bloqueados_void){
	t_entrenadores_bloqueados *bloqueados_2 = (t_entrenadores_bloqueados *) entrenadores_bloqueados_void;
	int size = queue_size(bloqueados_2->entrenadores);
	log_info(mapa_log, "Entrenadores Bloqueados del Pokenest %s: %d.", key, size);
	t_list *entrenadores_list = (t_list *) bloqueados_2->entrenadores->elements;
	list_add_all(bloqueados, entrenadores_list);
	log_info(mapa_log, "Agregue los Entrenadores Bloqueados del Pokenest %s.", key);
}

void entregar_pokemon(t_entrenador* entrenador, t_pokemon_mapa *pokemon, char pokenest_id){
	list_add(entrenador->pokemons, pokemon);
	//t_list* pokenests = get_listado_pokenest(ruta_pokedex, nombre_mapa);
	t_pokenest* pokenest = get_pokenest_by_identificador(lista_pokenests, pokenest_id);
	//char* pokenest_dir = get_pokenest_path_dir(ruta_pokedex , nombre_mapa);
	char* pokemon_path = string_new();
	string_append(&pokemon_path, pokenest_dir);
	string_append(&pokemon_path,"/");
	string_append(&pokemon_path,pokenest->nombre);
	string_append(&pokemon_path,"/");
	string_append(&pokemon_path, pokemon->nombre_archivo);
	int tamanio_nombre_archivo = string_length(pokemon_path); //TODO: VER SI SUMAR Sumar 1
	enviarInt(entrenador->fd, POKEMON_CONCEDIDO);
	enviarInt(entrenador->fd, (sizeof(char) * tamanio_nombre_archivo));
	if(enviarMensaje(entrenador->fd, pokemon_path) < 0){
		//Consideramos que el Entrenador se Deconecto - Hay que liberar recursos
		liberar_recursos_entrenador(entrenador, 0);
	} else {
		free(entrenador->pokemon_bloqueado); //Limpio el ID del Pokemon porque ya se otorgo
		entrenador->pokemon_bloqueado = string_new();
		entrenador->conoce_ubicacion = false; //20161214 - FM
		log_info(mapa_log, "Se otorgo el Pokemon %s al Entrenador %s.", pokemon->nombre, entrenador->nombre);
		agregar_entrenador_a_listos(entrenador);
		//Resto el Recurso de la Interfaz de Mapa
		disminuir_recursos_de_pokenest(items, pokenest_id, nombre_mapa);
	}
	//free(pokenest_dir);
	free(pokemon_path);
}

void entregar_medalla(t_entrenador *entrenador, char* nombre_mapa){
	log_info(mapa_log, "Se va a Entregar la Medalla a %s.", entrenador->nombre);
	char *ruta_medalla = get_medalla_path(ruta_pokedex, nombre_mapa);
	log_info(mapa_log, "La ruta de la Medalla es %s.", ruta_medalla);
	enviarInt(entrenador->fd, strlen(ruta_medalla));
	enviarMensaje(entrenador->fd, ruta_medalla);

	//Libero Entrenador
	free(ruta_medalla);
	liberar_recursos_entrenador(entrenador, 0);
}

void enviar_posicion_pokenest(t_entrenador* entrenador , t_mensajes *mensajes){ //20161214 - FM
	char *nombre_pokenest = (char *)obtener_mensaje(mensajes);
	pthread_mutex_lock(&mutex_pokenests);
	t_pokenest *pokenest = get_pokenest_by_identificador(lista_pokenests, *nombre_pokenest);
	pthread_mutex_unlock(&mutex_pokenests);
	log_info(mapa_log, "Se envia posicion del Pokenest: %s", pokenest->nombre);
	enviarInt(entrenador->fd,pokenest->posicion->x);
	enviarInt(entrenador->fd,pokenest->posicion->y);

	entrenador->pokenest = pokenest->posicion;
	entrenador->conoce_ubicacion = true;
	free(nombre_pokenest);
}

void avanzar_hacia_pokenest(t_entrenador *entrenador, t_mensajes *mensajes){
	int *avance_x = (int*) obtener_mensaje(mensajes);
	//entrenador->posicion->x = (int) obtener_mensaje(mensajes);
	entrenador->posicion->x = *avance_x;
	mover_entrenador_en_mapa(items, entrenador, nombre_mapa);

	int *avance_y = (int*) obtener_mensaje(mensajes);
	//entrenador->posicion->y = (int) obtener_mensaje(mensajes);
	entrenador->posicion->y = *avance_y;
	mover_entrenador_en_mapa(items, entrenador, nombre_mapa);

	free(avance_x);
	free(avance_y);

	log_info(mapa_log, "El Entrenador %s se movio a la posicion (%d, %d).", entrenador->nombre, entrenador->posicion->x, entrenador->posicion->y);
}

void solicitar_atrapar_pokemon(t_entrenador *entrenador, t_mensajes *mensajes){
	char *nombre_pokemon = (char *)obtener_mensaje(mensajes);
	string_append(&entrenador->pokemon_bloqueado, nombre_pokemon);
	free(nombre_pokemon);
	log_info(mapa_log, "El Entrenador %s solicito atrapar a un Pokemon.", entrenador->nombre);
}

/********* FUNCION ENCARGADA DEL MANEJO DE LAS SYSTEM CALLS*********/
void system_call_manager(){
	//Manejo de System Calls
	signal(SIGUSR2, system_call_catch);
	signal(SIGINT, system_call_catch);
	signal(SIGPIPE, system_call_catch);
}

void system_call_catch(int signal){

	if (signal == SIGUSR2){
		metadata = get_mapa_metadata(ruta_pokedex, nombre_mapa);
		log_info(mapa_log, "Se ha enviado la señal SIGUSR2. Se actualizarán las variables de Configuración.");
		set_algoritmoActual();
		set_quantum();
		set_retardo();
		set_interbloqueo();
		log_info(mapa_log, "Nuenas variables de Conf: Algoritmo: %s - Quantum: %d - Retardo de Interbloqueo: %d. - Retardo de Quantum: %d." , algoritmo, mapa_quantum, interbloqueo, retardo);
	}
	//Ctrl + C
	if(signal ==  SIGINT){
		liberar_mensajes(mensajes_entrenadores, mapa_log);
		free(log_nombre);
		free(algoritmo);
		free(nombre_mapa);
		free(ruta_pokedex);
		free(pokenest_dir);
		liberar_pokenest(lista_pokenests);
		finalizar_mapa();
		list_destroy(items);
		log_destroy(mapa_log);
		exit(1);
	}

	if(signal == SIGPIPE){
		log_error(mapa_log, "Se envio un mensaje a un Entrenador que se ha desconectado.");
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
			es_algoritmo_rr = FALSE;
		}

		//Verifico si el entrenador no se desconecto -- Esta validacion se podria borrar
		if(fcntl(entrenador->fd, F_GETFL) != -1){
			atender_Viaje_Entrenador(entrenador, es_algoritmo_rr);
		} else {
			log_info(mapa_log, "El Entrenador %s se ha desconectado del Mapa.", entrenador->nombre);
			liberar_recursos_entrenador(entrenador, 0);
		}
	}

	free(round_robin);
}

void atender_Viaje_Entrenador(t_entrenador* entrenador, bool es_algoritmo_rr){
	int turnos = 0;
	bool bloqueado = FALSE; //Flag utilizado para saber si un Entrenador se Bloqueo
	bool finalizo = FALSE; //Falg utilizado para saber si el Entrenador Finalizo
	bool desconectado = FALSE; //Falg utilizado para saber si el Entrenador se Desconecto
	int *instruccion;
	bool conocio_ubicacion = false;

	while ((!es_algoritmo_rr || turnos < mapa_quantum) && !bloqueado && !finalizo && !desconectado && !conocio_ubicacion){
		t_mensajes *mensajes_entrenador = obtener_mensajes_de_entrenador(mensajes_entrenadores, entrenador->fd);
		//sem_wait(&sem_mensajes);
		sem_wait(&mensajes_entrenador->semaforo);

		log_info(mapa_log, "Al Entrenador %s le toca el turno %d", entrenador->nombre, turnos);

		//t_mensajes *mensajes_entrenador = obtener_mensajes_de_entrenador(mensajes_entrenadores, entrenador->fd);
		instruccion = (int*) obtener_mensaje(mensajes_entrenador);

		switch(*instruccion){
			case UBICACION_POKENEST:
				if(!es_algoritmo_rr)
					conocio_ubicacion = true;
				enviar_posicion_pokenest(entrenador, mensajes_entrenador);//20161214 - FM
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
			case DESCONECTADO:
				log_info(mapa_log, "El Entrenador %s se ha desconectado del Mapa.", entrenador->nombre);
				desconectado = TRUE;
				break;
			default:
				log_error(mapa_log, "Se ha producido un error al tratar de atender instruccion del Entrenador.");
				break;
		}

		free(instruccion);

		turnos++;

		usleep(retardo);
	}

	//Finalizo el Turno del Entrenador
	if(!bloqueado){
		if(!finalizo && !desconectado){
			agregar_entrenador_a_listos(entrenador);
		}

		if(desconectado){
			liberar_recursos_entrenador(entrenador, 0);
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

int menor_distancia(t_entrenador* unEntrenador, t_entrenador* otroEntrenador){
	double unaDistancia = 0;
	double otraDistancia = 0;

	unaDistancia = distancia_a_pokenest(unEntrenador);
	otraDistancia = distancia_a_pokenest(otroEntrenador);

	if(unaDistancia < otraDistancia)
		return (bool)true;
	else
		return (bool)false;
}

double distancia_a_pokenest(t_entrenador* entrenador){
	double deltaTotal = 0;
	double deltaX = 0;
	double deltaY = 0;

	deltaX = entrenador->pokenest->x - entrenador->posicion->x;
	deltaY = entrenador->pokenest->y - entrenador->posicion->y;

	deltaTotal = deltaX + deltaY; //sqrt(deltaX*deltaX + deltaY*deltaY);

	return deltaTotal;

}

void srdf(t_list* entrenadores_listos){
	bool (*pf)(t_entrenador*,t_entrenador*) = menor_distancia;
	list_sort(entrenadores_listos, (void*)pf);
}

/* Actualizacion de valores desde el Archivo de comfiguracion */
void set_algoritmoActual(){
	algoritmo = get_mapa_algoritmo(metadata);
}
void set_quantum(){
	mapa_quantum = get_mapa_quantum(metadata) * 1000;
}

void set_retardo(){
	retardo = get_mapa_retardo(metadata) * 1000;
}
void set_interbloqueo(){
	interbloqueo = get_mapa_tiempo_deadlock(metadata);
}

void administrar_bloqueados(char *pokenest_id){
	//Obtengo la Estructura de Cola de Entrenadores Bloqueados
	pthread_mutex_lock(&mutex_cola_bloqueados);
	t_entrenadores_bloqueados *ent_bloqueados = (t_entrenadores_bloqueados *) dictionary_get(entrenadores_bloqueados, pokenest_id);
	pthread_mutex_unlock(&mutex_cola_bloqueados);

	t_queue *entrenadores = ent_bloqueados->entrenadores;

	while(1){
		sem_wait(&sem_pokemones_disponibles[ent_bloqueados->sem_index]);
		sem_wait(&sem_entrenadores_bloqueados[ent_bloqueados->sem_index]);
		log_info(mapa_log, "Ejecuto Hilo Bloqueado de Pokenest %s.", pokenest_id);
		//Obtengo el Entrenador Bloqueado
		pthread_mutex_lock(&mutex_cola_bloqueados);
		t_entrenador *entrenador = queue_pop(entrenadores);
		pthread_mutex_unlock(&mutex_cola_bloqueados);
		log_info(mapa_log, "Se removio al Entrenador %s de la Cola de Bloqueados.", entrenador->nombre);

		//Obtengo el Pokemon
		pthread_mutex_lock(&mutex_pokenests);
		t_pokemon_mapa *pokemon = get_pokemon_by_identificador(lista_pokenests, *pokenest_id);
		pthread_mutex_unlock(&mutex_pokenests);

		//Entregamos el Pokemon al Entrenador
		entregar_pokemon(entrenador, pokemon, *pokenest_id);
	}
}

void incializar_gestion_colas_bloqueados(){
	entrenadores_bloqueados = dictionary_create();
	int cantidad_pokenest = list_size(lista_pokenests);
	//pthread_t *bloqueados_threads_vec = malloc(cantidad_pokenest * sizeof(pthread_t));
	bloqueados_threads_vec = malloc(cantidad_pokenest * sizeof(pthread_t));
	sem_entrenadores_bloqueados = malloc(cantidad_pokenest * sizeof(sem_t));
	sem_pokemones_disponibles = malloc(cantidad_pokenest * sizeof(sem_t));
	int i = 0;
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

void liberar_recursos_entrenador(t_entrenador *entrenador, int mensaje){
	//wait_de_mensajes(mensajes_entrenadores, entrenador->fd, &sem_mensajes);
	int cant_pokemons = list_size(entrenador->pokemons);
	int i = 0, fd;
	log_info(mapa_log, "Voy a liberar los recursos del Entrenador %s.", entrenador->nombre);

	pthread_mutex_lock(&mutex_pokenests);
	t_pokemon_mapa *pokemon;
	for(i = 0; i < cant_pokemons; i++){
		pokemon = (t_pokemon_mapa *)list_remove(entrenador->pokemons, 0); //Siempre saco el Pokemon que quedo en la posicion 0
		log_info(mapa_log, "Voy a sumar un Recurso del Pokemon %s.", pokemon->nombre);
		//Actualizo la interfaz Grafica sumando un recurso al Pokenest
		aumentar_recursos_de_pokenest(items, pokemon->pokenest_id, nombre_mapa);

		//Agrego el Pokemon al Pokenest
		sumar_recurso_pokemon(pokemon);
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
	free(entrenador->nombre);
	free(entrenador->pokemon_bloqueado);
	free(entrenador);

	switch(mensaje){
		case MUERTE:
			log_info(mapa_log, "Envio el aviso de MUERTE al Entrenador del FD %d.", fd);
			//Informo de la Muerte al Entrenador
			enviarInt(fd, MUERTE);
	}
}

/* Funciones para la Gestion de Entrenadores Interbloqueados */
void add_entrenadores_interbloqueados(char *key, void *entrenadores_bloqueados_void){
	t_entrenadores_bloqueados *pokenest_bloqueados = (t_entrenadores_bloqueados *) entrenadores_bloqueados_void;
	t_list *entrenadores = (t_list *) pokenest_bloqueados->entrenadores->elements;
	int cant_entrenadores = list_size(entrenadores);
	int cant_pokenets = list_size(lista_pokenests);
	int i = 0, j = 0, cant_pokemons = 0, pokenest_index = 0;

	log_trace(mapa_log, "Se agregan los Entrenadores Bloqueados del Pokenest %s.", key);

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
			t_pokemon_mapa *pokemon = (t_pokemon_mapa *) list_get(interbloqueado->entrenador->pokemons, j);
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
	int *disponibles;
	int cant_pokenets = list_size(lista_pokenests);
	void (*add_entrenadores_interbloqueados_iterator) (char*, void*) = add_entrenadores_interbloqueados;
	int i = 0, j = 0, cant_entrenadores = 0, modo_batalla;
	t_list *entrenadores_interbloqueados;

	while(1){
		usleep(interbloqueo);
		//pthread_mutex_lock(&mutex_cola_bloqueados);

		i = 0;
		j = 0;
		cant_entrenadores = 0;
		disponibles = malloc(cant_pokenets * sizeof(int));
		lista_interbloqueo = list_create();
		entrenadores_interbloqueados = list_create();
		modo_batalla = get_mapa_batalla_on_off(metadata);

		log_trace(mapa_log, "Se va a realizar el Chequeo de Entrenadores Interbloqueados.");

		//Preparo el Array de Disponobiles
		pthread_mutex_lock(&mutex_pokenests);
		for(i = 0; i < cant_pokenets; i++){
			t_pokenest *pokenets = (t_pokenest *) list_get(lista_pokenests, i);
			disponibles[i] = pokenets->cantPokemons;
		}
		pthread_mutex_unlock(&mutex_pokenests);

		char *char_disponibles= string_new();
		for(i = 0; i < cant_pokenets; i++){
			char *disponible_i = string_itoa(disponibles[i]);
			string_append(&char_disponibles, " ");
			string_append(&char_disponibles, disponible_i);
			free(disponible_i);
		}

		log_info(mapa_log, "Recursos Disponibles: %s.", char_disponibles);

		free(char_disponibles);

		//Preparo la estructura de Entrenadores
		pthread_mutex_lock(&mutex_cola_bloqueados);
		dictionary_iterator(entrenadores_bloqueados, add_entrenadores_interbloqueados_iterator);
		pthread_mutex_unlock(&mutex_cola_bloqueados);

		cant_entrenadores = list_size(lista_interbloqueo);
		//Verifico si el Entrenador no Esta Marcado
		log_info(mapa_log, "Cantidad de Entrenadores Bloqueados: %d.", cant_entrenadores);

		if(cant_entrenadores > 0){
			//Marcos los entrenadores que no tienen recursos asignados
			for(i = 0; i < cant_entrenadores; i++){
				t_entrenador_interbloqueado *entrenador = (t_entrenador_interbloqueado *) list_get(lista_interbloqueo, i);

				if(chequear_pokemones_sin_asignar(cant_pokenets, entrenador->asignacion)){
					entrenador->marcado = 1; //Marco al Entrenador
				} else {
					entrenador->marcado = 0;
				}
			}

			log_info(mapa_log, "Se marcaron a los Entrenadores que no tienen Recursos asignados.");

			//Busco los Entrenadores que no fueron marcados usando su matriz de solicitud
			int termino_ciclo = 0;

			while(termino_ciclo != 1){
				if(recorrer_solicitudes(lista_interbloqueo, cant_pokenets, cant_entrenadores, disponibles) == 0){
					termino_ciclo = 1;
				}
			}

			log_info(mapa_log, "Se busca los Entrenadores que no fueron marcados.");

			//Libero la lista de Chequeo de Interbloqueo
			for(i = 0; i < cant_entrenadores; i++){
				t_entrenador_interbloqueado *entrenador = (t_entrenador_interbloqueado *) list_remove(lista_interbloqueo, 0);

				if(entrenador->marcado == 0){
					list_add(entrenadores_interbloqueados, entrenador->entrenador);
				}

				char *char_asignacion = string_new();
				char *char_solicitud = string_new();
				for(j = 0; j < cant_pokenets; j++){
					string_append(&char_asignacion, " ");
					string_append(&char_asignacion, string_itoa(entrenador->asignacion[j]));
					string_append(&char_solicitud, " ");
					string_append(&char_solicitud, string_itoa(entrenador->solicitud[j]));
				}
				log_info(mapa_log, "Recursos de %s - Asignacion: %s. Solicitud: %s", entrenador->entrenador->nombre, char_asignacion, char_solicitud);
				free(char_asignacion);
				free(char_solicitud);

				entrenador->entrenador = NULL;
				free(entrenador->asignacion);
				free(entrenador->entrenador);
				free(entrenador->solicitud);
				free(entrenador);
			}

			log_info(mapa_log, "Se va a verificar la cantidad de Entrenadores Intebloqueados.");
			if(list_size(entrenadores_interbloqueados) > 1){
				ordenar_entrenadores_interbloqueados(entrenadores_interbloqueados);

				//Verifico si el Mapa se encuentra en modo batalla
				if(modo_batalla){
					log_info(mapa_log, "Se va a comenzar con la Batalla!!!.");
					//pthread_mutex_lock(&mutex_cola_bloqueados);
					batalla_pokemon(entrenadores_interbloqueados);
					//pthread_mutex_unlock(&mutex_cola_bloqueados);
				}

			} else {
				log_info(mapa_log, "No se registran Entrenadores Interbloqueados.");
			}

		} else {
			log_info(mapa_log, "No se registran Entrenadores Bloqueados.");
		}

		list_destroy(entrenadores_interbloqueados);
		list_destroy(lista_interbloqueo);
		free(disponibles);

		//pthread_mutex_unlock(&mutex_cola_bloqueados);
	}
}

void batalla_pokemon(t_list *entrenadores){

	int entrenadores_size = list_size(entrenadores);
	log_info(mapa_log, "Se enfrentarán %d Entrenadores Pokemon!.", entrenadores_size);

	t_entrenador *victima = (t_entrenador *)list_remove(entrenadores, 0);

	while(list_size(entrenadores) > 0){
		t_entrenador *adversario = list_remove(entrenadores, 0);
		log_info(mapa_log, "Se enfrentarán los Entrenadores %s y %s.", victima->nombre, adversario->nombre);
		t_entrenador *perdedor = liberar_batalla(victima, adversario, mapa_log);
		t_entrenador *ganador = NULL;

		if(perdedor->id == victima->id){
			ganador = adversario;
		} else {
			ganador = victima;
		}

		log_info(mapa_log, "El Entrenador %s ha perdido contra el Entrenador %s.", perdedor->nombre, ganador->nombre);
		enviar_resultado_batalla(perdedor, ganador); //TODO: Descomentar para mandar el aviso
		victima = perdedor; //Preparo al perdedor para el Siguiente enfrentamiento
	}

	log_info(mapa_log, "El Entrenador %s ha resultado el Perdedor de la Batalla Pokemon.", victima->nombre);

	log_info(mapa_log, "Remuevo Entrenador %s de Bloqueados.", victima->nombre);
	//Remuevo de los Entrenadores Bloqueados
	pthread_mutex_lock(&mutex_cola_bloqueados);
	victima = remover_entrenador_de_bloqueados(victima);
	pthread_mutex_unlock(&mutex_cola_bloqueados);

	log_info(mapa_log, "Libero los recursos del Entrenador %s.", victima->nombre);
	//Libero los Recursos del Mismo
	liberar_recursos_entrenador(victima, MUERTE);
}

void enviar_resultado_batalla(t_entrenador *perdedor, t_entrenador *ganador){
	char *mensaje_ganador = string_new();
	char *mensaje_perdedor = string_new();

	string_append(&mensaje_ganador, "Ganaste la Batalla contra el Entrenador ");
	string_append(&mensaje_perdedor, "Perdiste la Batalla contra el Entrenador ");

	string_append(&mensaje_ganador, perdedor->nombre);
	string_append(&mensaje_perdedor, ganador->nombre);

	int mensaje_ganador_size = string_length(mensaje_ganador); //todo ver si sumar 1
	int mensaje_perdedor_size = string_length(mensaje_perdedor);

	//Envio el Mensaje al Ganador
	enviarInt(ganador->fd, VICTORIA);
	enviarInt(ganador->fd, mensaje_ganador_size);
	enviarMensaje(ganador->fd, mensaje_ganador);

	//Envio el Mensaje al Perdedor
	enviarInt(perdedor->fd, DERROTA);
	enviarInt(perdedor->fd, mensaje_perdedor_size);
	enviarMensaje(perdedor->fd, mensaje_perdedor);

	free(mensaje_ganador);
	free(mensaje_perdedor);
}
