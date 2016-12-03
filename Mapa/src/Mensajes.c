#include "Mensajes.h"

//Mutexs Para Lista de Mensajes
pthread_mutex_t mutex_mensajes;

void inicializar_semaforo_mensajes(){
	pthread_mutex_init(&mutex_mensajes, NULL);
}

void inicializar_mensajes_entrenador(t_list *mensajes, int fd, t_log *log){
	t_mensajes *cola = malloc(sizeof(t_mensajes));
	cola->fd = fd;
	cola->mensajes = queue_create();

	pthread_mutex_lock(&mutex_mensajes);
	list_add(mensajes, cola);
	pthread_mutex_unlock(&mutex_mensajes);
	log_info(log, "Se ha creado exitosamente la estructura de cola de mensajes para el FD %d.", fd);
}

void agregar_nuevo_mensaje(t_list *mensajes_entrenadores, int fd, void *mensaje){
	pthread_mutex_lock(&mutex_mensajes);
	int size = list_size(mensajes_entrenadores);
	int i = 0;

	for(i = 0; i < size; i++){
		t_mensajes *mensajes = list_get(mensajes_entrenadores, i);

		if(mensajes->fd == fd){
			queue_push(mensajes->mensajes, mensaje);
		}
	}
	pthread_mutex_unlock(&mutex_mensajes);
}

void *obtener_mensaje(t_mensajes *mensajes){
	pthread_mutex_lock(&mutex_mensajes);
	void *mensaje = queue_pop(mensajes->mensajes);
	pthread_mutex_unlock(&mutex_mensajes);
	return mensaje;
}

t_mensajes *obtener_mensajes_de_entrenador(t_list *mensajes_entrenadores, int fd){
	pthread_mutex_lock(&mutex_mensajes);
	int size = list_size(mensajes_entrenadores);
	int i = 0;
	t_mensajes *mensajes = NULL;
	for(i = 0; i < size; i++){
		t_mensajes *mensajes_entrenador = list_get(mensajes_entrenadores, i);

		if(mensajes_entrenador->fd == fd){
			mensajes = mensajes_entrenador;
		}
	}
	pthread_mutex_unlock(&mutex_mensajes);

	return mensajes;
}


int recibir_mensaje_ubicacion_pokenest(t_list *mensajes_entrenadores, int fd, t_log *log){
	t_mensajes *mensajes_entrenador = obtener_mensajes_de_entrenador(mensajes_entrenadores, fd);

	if(mensajes_entrenador != NULL){
		t_queue *cola_mensajes = mensajes_entrenador->mensajes;
		int tamanio_texto;
		int *result = malloc(sizeof(int));
		char *nombre_pokenest;
		int accion = UBICACION_POKENEST;

		//Recibo el Nombre del Pokenest
		tamanio_texto = recibirInt(fd, result, log);

		if(*result <= 0){
			free(result);
			return 0;
		}

		nombre_pokenest = malloc(sizeof(char) * tamanio_texto);
		recibirMensaje(fd, nombre_pokenest, tamanio_texto, log);

		pthread_mutex_lock(&mutex_mensajes);
		queue_push(cola_mensajes, accion);
		queue_push(cola_mensajes, nombre_pokenest);
		pthread_mutex_unlock(&mutex_mensajes);

		free(result);

		return 1;
	}

	return 0;
}

int recibir_mensaje_avanzar_hacia_pokenest(t_list *mensajes_entrenadores, int fd, t_log *log){

	t_mensajes *mensajes_entrenador = obtener_mensajes_de_entrenador(mensajes_entrenadores, fd);

	if(mensajes_entrenador != NULL){
		t_queue *cola_mensajes = mensajes_entrenador->mensajes;
		int *result = malloc(sizeof(int));
		int avance_x, avance_y;
		int accion = AVANZAR_HACIA_POKENEST;

		//Recibo el Avance en X
		avance_x = recibirInt(fd, result, log);

		if(*result <= 0){
			free(result);
			return 0;
		}

		//Recibo el Avance en Y
		avance_y = recibirInt(fd, result, log);

		if(*result <= 0){
			free(result);
			return 0;
		}


		pthread_mutex_lock(&mutex_mensajes);
		queue_push(cola_mensajes, accion);
		queue_push(cola_mensajes, avance_x);
		queue_push(cola_mensajes, avance_y);
		pthread_mutex_unlock(&mutex_mensajes);

		free(result);

		return 1;
	}

	return 0;
}

int recibir_mensaje_atrapar_pokemon(t_list *mensajes_entrenadores, int fd, t_log *log){
	t_mensajes *mensajes_entrenador = obtener_mensajes_de_entrenador(mensajes_entrenadores, fd);

	if(mensajes_entrenador != NULL){
		t_queue *cola_mensajes = mensajes_entrenador->mensajes;
		int accion = ATRAPAR_POKEMON;
		int tamanio_nombre_pokemon;
		int *result = malloc(sizeof(int));
		char *nombre_pokemon;

		//Recibo el Nombre del Pokemon
		tamanio_nombre_pokemon = recibirInt(fd, result, log);

		if(*result <= 0){
			free(result);
			return 0;
		}

		nombre_pokemon = malloc(sizeof(char) * tamanio_nombre_pokemon);
		recibirMensaje(fd, nombre_pokemon, tamanio_nombre_pokemon, log);


		pthread_mutex_lock(&mutex_mensajes);
		queue_push(cola_mensajes, accion);
		queue_push(cola_mensajes, nombre_pokemon);
		pthread_mutex_unlock(&mutex_mensajes);

		free(result);

		return 1;

	}

	return 0;
}

int recibir_mensaje_objetivo_cumplido(t_list *mensajes_entrenadores, int fd, t_log *log){
	t_mensajes *mensajes_entrenador = obtener_mensajes_de_entrenador(mensajes_entrenadores, fd);

	if(mensajes_entrenador != NULL){
		t_queue *cola_mensajes = mensajes_entrenador->mensajes;
		int accion = OBJETIVO_CUMPLIDO;

		pthread_mutex_lock(&mutex_mensajes);
		queue_push(cola_mensajes, accion);
		pthread_mutex_unlock(&mutex_mensajes);

		return 1;

	}

	return 0;
}

void eliminar_cola_mensajes_entrenador(t_list *mensajes, int fd, t_log *log){
	pthread_mutex_lock(&mutex_mensajes);

	int mensajes_size = list_size(mensajes);
	int i = 0, borrado = 0;

	while(i < mensajes_size && !borrado){
		t_mensajes *mensaje = (t_mensajes *) list_get(mensajes, i);
		//Borro todos los mensajes
		if(mensaje->fd == fd){
			list_remove(mensajes, i);
			queue_destroy(mensaje->mensajes);
			free(mensaje);
			borrado = 1;
			log_info(log, "Se ha borrado exitosamente la Estructura de Mensajes para el FD N° %d.", fd);
		}
		i++;
	}

	pthread_mutex_unlock(&mutex_mensajes);
}

void entrenador_desconectado(t_list *mensajes_entrenadores, int fd){
	t_mensajes *mensajes_entrenador = obtener_mensajes_de_entrenador(mensajes_entrenadores, fd);

	if(mensajes_entrenador != NULL){
		t_queue *cola_mensajes = mensajes_entrenador->mensajes;
		int accion = DESCONECTADO;

		pthread_mutex_lock(&mutex_mensajes);
		queue_push(cola_mensajes, accion);
		pthread_mutex_unlock(&mutex_mensajes);
	}
}
