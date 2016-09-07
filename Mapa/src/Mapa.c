
#include "Mapa.h"

//Estructuras para el Manejo de Entrenadores
t_queue *entrenadores_listos;
t_list *entrenadores; //TODO: Ver si puede llegar a servir

//Log
t_log *mapa_log;

int main(void) {

	// Creacion del Log
	//char *log_level = config_get_string_value(mapa_log , LOG_LEVEL);
	char *log_level = "INFO";
	mapa_log = CreacionLogWithLevel(log_nombre, programa_nombre, log_level);
	log_info(mapa_log, "Se ha creador el Log para el Mapa.");

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

	//Espero conexiones y pedidos de Entrenadores
	while(1){
		fd_sets_entrenadores->readFileDescriptorSet = fd_sets_entrenadores->masterSet;

		if (select(fd_sets_entrenadores->maxFileDescriptorNumber + 1,
						&(fd_sets_entrenadores->readFileDescriptorSet),
						&(fd_sets_entrenadores->writeFileDescriptorSet),
						NULL, NULL) == -1){
			perror("Se ha producido un error al intentar atender las peticiones de los Entrenadores.");
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
		default:
			log_error(mapa_log, "Se ha producido un error al intentar atender a la peticion del Entrenador.");
			break;
	}
}

void recibir_nuevo_entrenador(int fd){
	t_entrenador *entrenador = malloc(sizeof(t_entrenador));
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

	entrenador->fd = fd;
	entrenador->nombre = nombre;
	entrenador->caracter = (char) caracter;

	list_add(entrenadores, entrenador);

	printf("Bienvenido Entrenador %s N° %d. \n", entrenador->nombre, fd);
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

	printf("Mensaje recibido del socket %d: %s", fd, texto);

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