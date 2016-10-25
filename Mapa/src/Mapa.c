
#include "Mapa.h"

//Mutexs de Estructura de Estados
pthread_mutex_t mutex_entrenadores_listos;
pthread_mutex_t mutex_cola_bloqueados;
pthread_mutex_t mutex_entrenadores_ejecutando;

//Semaforos para sincronizar Hilos
sem_t sem_listos;
sem_t sem_bloqueados;
sem_t sem_ejecutando;

//Estructuras para el Manejo de Entrenadores
t_list *entrenadores_listos;
t_list *entrenadores_bloqueados;
t_list *entrenadores_ejecutando;

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

//Nombre de Mapa
char *nombre_mapa;

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

	nombre_mapa= string_new();

	if (argv[1] == NULL || argv[2] == NULL ){
		perror("Asegurese de ingresar el nombre del Mapa y la ruta del pokedex");
		exit(1);
	}

	string_append(&nombre_mapa, argv[1]);

	metadata = get_mapa_metadata(argv[2] , nombre_mapa);
	lista_pokenests = get_listado_pokenest(argv[2] , nombre_mapa); //Obtengo un listado de todos los Pokenests con sus respectivos Pokemons

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

	//Lista de nombres de pokenests
	//pokenests=get_lista_de_pokenest(metadata);
	//cargar_pokenests_en _items(items, argv[2], nombre_mapa, pokenests);

	dibujar_mapa_vacio(items);

	//Hilo Hoja de viaje
	pthread_t lista_de_bloqueados;
	pthread_create(&lista_de_bloqueados, NULL, (void *) administrar_bloqueados,NULL);

	//Hilo planificador
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
	log_info(mapa_log, "Se agrega un entrenador a la cola de listos");
}

//Remover entrenador segun Round Robin
t_entrenador *remover_entrenador_listo_por_RR(){
	t_entrenador *entrenador = NULL;
	pthread_mutex_lock(&mutex_entrenadores_listos);
	//sem_wait(&sem_listos);
	entrenador = (t_entrenador *) list_remove(entrenadores_listos, 0);
	pthread_mutex_unlock(&mutex_entrenadores_listos);
	log_info(mapa_log, "Se remueve un entrenador de la lista de listos");
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
	log_info(mapa_log, "Se remueve un entrenador de la lista de listos");
	return entrenador;

}

//Agrega un entrenador a la cola bloqueados
void agregar_entrenador_a_bloqueados(t_entrenador *entrenador){
	pthread_mutex_lock(&mutex_cola_bloqueados);
	list_add(entrenadores_bloqueados, (void *) entrenador);
	sem_post(&sem_bloqueados);
	pthread_mutex_unlock(&mutex_cola_bloqueados);
	log_info(mapa_log, "Se agrega un entrenador a la lista de bloqueados");
}

//Elimina un entrenador de la lista de bloqueados
void remover_entrenador_de_bloqueados(int i){
	pthread_mutex_lock(&mutex_cola_bloqueados);
	list_remove(entrenadores_bloqueados,i);
	//sem_wait(&sem_bloqueados);
	pthread_mutex_unlock(&mutex_cola_bloqueados);
	log_info(mapa_log, "Se remueve un entrenador de la lista de bloqueados");
}

//Agrega un entrenador a la lista de ejecutando
void agregar_entrenador_a_ejecutando(t_entrenador *entrenador){
	pthread_mutex_lock(&mutex_entrenadores_ejecutando);
	list_add(entrenadores_ejecutando, (void *) entrenador);
	sem_post(&sem_ejecutando);
	pthread_mutex_unlock(&mutex_entrenadores_ejecutando);
	log_info(mapa_log, "Se agrega un entrenador a la lista de ejecutando");
}

//Elimina un entrenador de la lista de ejecutando
void remover_entrenador_de_ejecutando(int i){
	pthread_mutex_lock(&mutex_entrenadores_ejecutando);
	sem_wait(&sem_ejecutando);
	list_remove(entrenadores_ejecutando,i);
	pthread_mutex_unlock(&mutex_entrenadores_ejecutando);
	log_info(mapa_log, "Se remueve un entrenador de la lista de ejecutando");
}

/********* FUNCIONES DE INICIALIZACION *********/
void inicializar_estructuras(){

	//Estructuras
	entrenadores_listos = list_create();
	entrenadores_bloqueados = list_create();
	entrenadores_ejecutando = list_create();
	items = list_create();

	datos_mapa = malloc(sizeof(t_datos_mapa));
	algoritmo=malloc(sizeof(char)*4);
	datos_mapa->items = items;
	datos_mapa->entrenador = NULL;

	//Semaforos
	pthread_mutex_init(&mutex_desplaza_x, NULL);
	pthread_mutex_init(&mutex_desplaza_y, NULL);
	pthread_mutex_init(&mutex_entrenadores_listos, NULL);
	pthread_mutex_init(&mutex_cola_bloqueados, NULL);
	pthread_mutex_init(&mutex_recursos_pokenest,NULL);
	pthread_mutex_init(&mutex_entrenadores_ejecutando,NULL);

	sem_init(&sem_listos, 0, 0);
	sem_init(&sem_bloqueados,0,0);
	sem_init(&sem_ejecutando,0,0);

	log_info(mapa_log, "Se inicializaron las Estructuras y los Semaforos.");
}

/********* FUNCIONES PARA RECIBIR PETICIONES DE LOS ENTRENADORES *********/
void atender_entrenador(int fd_entrenador, int codigo_instruccion){

	int codigo = 0;
	int *result = malloc(sizeof(int));
	 codigo = recibirInt(fd_entrenador, result , mapa_log);

	if (*result > 0) {
	switch(codigo){
		case SOY_ENTRENADOR:
			recibir_nuevo_entrenador(fd_entrenador);
			break;
		case ENVIAR_MENSAJE:
			recibir_mensaje_entrenador(fd_entrenador);
			break;
		default:
			log_error(mapa_log, "Se ha producido un error al intentar atender a la peticion del Entrenador");
			break;
		}

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
	entrenador->id = entrenador_id + 1;
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
	//pthread_t trazado_mapa;
	//pthread_create(&trazado_mapa, NULL, (void*)mover_entrenador_hacia_recurso, (void*)datos_mapa);
		/***********************************************/

	//mover_entrenador_hacia_recurso(datos_mapa);

	/*****************************************************************/

	//Hilo Hoja de viaje
	//pthread_t entrenador_Hoja_De_Viaje;
	//pthread_create(&entrenador_Hoja_De_Viaje, NULL, (void *) atender_Viaje_Entrenador, entrenador);

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

void entregar_pokemon(t_entrenador* entrenador){

	//t_entrenador* entrenador=malloc (sizeof(t_entrenador));
	//entrenador=buscar_entrenador(fd);
	int tamanio_texto;
	int *result = malloc(sizeof(int));
	ITEM_NIVEL* pokenest= malloc(sizeof(ITEM_NIVEL));
	char *nombre_pokenest= NULL;
	char nombre;
	int i;

	//Recibo el mensaje TODO REUTILIZAR FUNCION RECIBIR_MENSAJE_ENTRENADOR
	tamanio_texto = recibirInt(entrenador->fd, result, mapa_log);
	nombre_pokenest= malloc(sizeof(char) * tamanio_texto);
	recibirMensaje(entrenador->fd, nombre_pokenest, tamanio_texto, mapa_log);
	nombre = *nombre_pokenest;
	pokenest = _search_item_by_id(items, nombre);
	pthread_mutex_lock(&mutex_recursos_pokenest);
	entrenador->pokemon_bloqueado=pokenest->id;
	pokenest->quantity=pokenest->quantity-1;
	pthread_mutex_unlock(&mutex_recursos_pokenest);
	agregar_entrenador_a_bloqueados(entrenador);
	//free(pokenest);
}

void entregar_medalla(t_entrenador *entrenador, char* nombre_mapa){
	char *ruta_medalla = string_new();
	string_append(&ruta_medalla, "mapas/");
	string_append(&ruta_medalla,nombre_mapa);
	string_append(&ruta_medalla,"/medalla-");
	string_append(&ruta_medalla,nombre_mapa);
	string_append(&ruta_medalla,".jpg");
	enviarInt(entrenador->fd,strlen(ruta_medalla));
	enviarMensaje(entrenador->fd,ruta_medalla);
	//TODO Liberar recursos del Entrenador

	free(entrenador);
}

void enviar_posicion_pokenest(int fd ){
	int tamanio_texto;
	int *result = malloc(sizeof(int));
	char *nombre_pokenest = NULL;
	char nombre;
	//Recibo el mensaje TODO REUTILIZAR FUNCION RECIBIR_MENSAJE_ENTRENADOR
	tamanio_texto = recibirInt(fd, result, mapa_log);
	nombre_pokenest = malloc(sizeof(char) * tamanio_texto);
	recibirMensaje(fd, nombre_pokenest, tamanio_texto, mapa_log);

	nombre = *nombre_pokenest;
	ITEM_NIVEL* pokenest= malloc(sizeof(ITEM_NIVEL));
	pokenest = _search_item_by_id(items, nombre);
	enviarInt(fd,pokenest->posx);
	enviarInt(fd,pokenest->posy);
	//free(result);
	//free(pokenest);
	//free(nombre_pokenest);

}

/********* FUNCION ENCARGADA DEL MANEJO DE LAS SYSTEM CALLS*********/
void system_call_catch(int signal){

	if (signal == SIGUSR2){
		log_info(mapa_log, "Se ha enviado la señal SIGUSR2. Se actualizarán las variables de Configuración.");
		/*TODO: Actualizar Algoritmo de planificación, valor de quantum, tiempo de retardo entre turnos y tiempo de chequeo de interbloqueo
		/Hacer esto cuando ya lo tengamos levantando desde el archivo de Configuración*/
		set_algoritmoActual();
		set_quantum();
		set_retardo();
		set_interbloqueo();

	}
}


void administrar_turnos() {
	t_entrenador* entrenador;
	ITEM_NIVEL * pokenest = malloc(sizeof(ITEM_NIVEL));
	int cantidad_bloqueados;
	int i;
	//char* algoritmo = malloc(sizeof(char)*4);
	char * algoritmo = string_new();
	char *round_robin = string_new();
	string_append(&round_robin, "RR");
	bool es_algoritmo_rr = FALSE;

	while (1){

		sem_wait(&sem_listos);
		//algoritmo = algoritmo_actual();

		string_append(&algoritmo, algoritmo_actual());

		//Si es verdadero, se extrae por algoritmo de Round Robin
		if(string_equals_ignore_case(algoritmo, round_robin)){
			entrenador = remover_entrenador_listo_por_RR();
			es_algoritmo_rr = TRUE;
		} else {
			entrenador = remover_entrenador_listo_por_SRDF();
			es_algoritmo_rr = FALSE;
		}

		atender_Viaje_Entrenador(entrenador, es_algoritmo_rr);

		sleep(retardo);

		/*if (algoritmo=="RR"){
			pthread_mutex_lock(&mutex_entrenadores_listos);
			if (list_size(entrenadores_listos)>0){
				entrenador=remover_entrenador_listo_por_RR();
				agregar_entrenador_a_ejecutando(entrenador);
				sleep(retardo);
				enviarInt(entrenador->fd,TURNO_CONCEDIDO);
				}
			pthread_mutex_unlock(&mutex_entrenadores_listos);
		} else {
			if (algoritmo=="SRDF"){
				entrenador=remover_entrenador_listo_por_SRDF();
				enviarInt(entrenador->fd,TURNO_CONCEDIDO);
			}
		}*/
	}

}

void atender_Viaje_Entrenador(t_entrenador* entrenador, bool es_algoritmo_rr){
	int turnos = 0;
	bool bloqueado = FALSE; //Flag utilizado para saber si un Entrenador se Bloqueo
	bool finalizo = FALSE; //Falg utilizado para saber si el Entrenador Finalizo
	int instruccion;
	int *result = malloc(sizeof(int));


	while ((!es_algoritmo_rr || turnos < mapa_quantum) && !bloqueado && !finalizo){
		//Envio al Entrenador el aviso que le toca realizar una accion
		enviarInt(entrenador->fd,TURNO_CONCEDIDO);
		instruccion = recibirInt(entrenador->fd, result, mapa_log);

		switch(instruccion){
			case UBICACION_POKENEST:
				enviar_posicion_pokenest(entrenador->fd);
				break;
			case AVANZAR_HACIA_POKENEST:
				mover_entrenador(entrenador, mapa_log, datos_mapa);
				break;
			case ATRAPAR_POKEMON:
				entregar_pokemon(entrenador);
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
	}

	//Finalizo el Turno del Entrenador
	if(!bloqueado){
		if(!finalizo){
			agregar_entrenador_a_listos(entrenador);
		}
	} else {
		agregar_entrenador_a_bloqueados(entrenador);
	}

	/*if (turnos>=mapa_quantum && estado!=ATRAPAR_POKEMON && estado!=OBJETIVO_CUMPLIDO){
		agregar_entrenador_a_listos(entrenador);
	} else{
		if (estado==ATRAPAR_POKEMON || estado==OBJETIVO_CUMPLIDO){
			estado=NULL;
		}
	}*/

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


void administrar_bloqueados(){
	t_entrenador* entrenador;
	ITEM_NIVEL * pokenest = malloc(sizeof(ITEM_NIVEL));
	//Ver esto de bloqueados tal vez en otro hilo
	while(1){
		sem_wait(&sem_bloqueados);
		entrenador = (t_entrenador *)list_get(entrenadores_bloqueados, 0);
		pokenest = _search_item_by_id(items, entrenador->pokemon_bloqueado);
		pthread_mutex_lock(&mutex_recursos_pokenest);
			if(pokenest->quantity > 0){
				pokenest->quantity--;
			remover_entrenador_de_bloqueados(0);
			enviarInt(entrenador->fd,POKEMON_CONCEDIDO);
			agregar_entrenador_a_listos(entrenador);
			}
		pthread_mutex_unlock(&mutex_recursos_pokenest);

		}

}
