#include "Entrenador.h"

//Log
t_log *entrenador_log;

//Configs
t_config * metadata;

t_entrenador *entrenador;
char *ruta_pokedex;
bool flag_fin_prog, flag_reinicio, flag_reconexion;

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
//void init_datos_entrenador(void);
void borrar(DIR*, char*, char*);
void borrar_del_mapa(char*, char*);


int main(int argc, char **argv) {


	flag_fin_prog = false;
	flag_reinicio = false;
	flag_reconexion = false;



	if (argv[1] == NULL || argv[2] == NULL){
		perror("Se necesita el nombre del entrenador y la ruta de la Pokedex!");
		exit(1);
	}

	nombre_entrendor = argv[1];
	ruta_pokedex = argv[2];

	posicion_pokenest = inicializar_posicion_pokenest();
	posicion_mapa =inicializar_posicion_entrenador();
	// Obtiene el archivo de metadata del entrenador.
	metadata = get_entrenador_metadata(ruta_pokedex, nombre_entrendor);
	init_datos_entrenador();

	//Obtiene hoja de viaje del archivo metadata del entrenador
	hojaDeViaje = get_entrenador_hoja_de_viaje(metadata);

	char *log_level = "INFO";
	entrenador_log = CreacionLogWithLevel(log_nombre, programa_nombre, log_level);
	log_info(entrenador_log, "Se ha creador el Log para el Entrenador.");

	//Manejo de System Calls
	signal(SIGTERM, system_call_catch);
	signal(SIGUSR1, system_call_catch);
	signal(SIGKILL,system_call_catch);

	//Comieza el viaje del Entreandor
	recorrer_hojaDeViaje(0);

	//Liberar memoria dinamica -> TODO: encapsular en funcion
	free(log_level);
	liberar_recursos();
	return EXIT_SUCCESS;
}


//Funcion encargada de Capturar las llamadas de Sistema
void system_call_catch(int signal){

	switch(signal){
	case SIGTERM:
		log_info(entrenador_log, "Se ha enviado la señal SIGTERM. Se le restará una vida al Entrenador.");

		if (!entrenador->vidas)
		reiniciar_Hoja_De_Viaje(0);
		else
			entrenador->vidas--;
		break;

	case SIGUSR1:
		log_info(entrenador_log, "Se ha enviado la señal SIGUSR1. Se le sumará una vida al Entrenador.");
		entrenador -> vidas ++;
		break;

	case SIGKILL:
		//flag_fin_prog = true;
		close(socket_mapa);
		break;

	default:
		break;
	}
}



int conectar_mapa(char* ruta_pokedex, char *mapa){
	ltn_sock_addinfo *ltn_cliente_entrenador;
	t_config * metadata_mapa = get_mapa_metadata(ruta_pokedex , mapa);
	char *mapa_puerto = string_itoa(get_mapa_puerto(metadata_mapa));
	char *mapa_ip = get_mapa_ip( metadata_mapa);
	ltn_cliente_entrenador = createClientSocket(mapa_ip, mapa_puerto);
	int socket_mapa = doConnect(ltn_cliente_entrenador);
	//TODO REVISAR FREE
	free(metadata_mapa);
	free(mapa_puerto);
	free(mapa_ip);
	//free(ltn_cliente_entrenador);
	//free(metadata_mapa);
	return socket_mapa;
}

void solicitar_posicion_pokenest(char *pokemon){
	int *result = malloc(sizeof(int));
	int posicion_x, posicion_y;
	enviarInt(socket_mapa, UBICACION_POKENEST);
	log_info(entrenador_log,  " ENVIE UBICACION_POKENEST");
	enviarInt(socket_mapa, (sizeof(char) * 2));
	log_info(entrenador_log , "ENVIE  2");
	enviarMensaje(socket_mapa,pokemon);
	log_info(entrenador_log, "%s", pokemon);
	posicion_x = recibirInt(socket_mapa, result, entrenador_log);
	log_info(entrenador_log,"%d",posicion_x);
	posicion_y = recibirInt(socket_mapa, result, entrenador_log);
	log_info(entrenador_log,"%d",posicion_y);
	//Actualizo la posicion del proximo pokenest
	actualizar_posicion_pokenest(posicion_pokenest, posicion_x, posicion_y);
	free(result);
}


void capturar_pokemon(char *nombre_pokemon, t_list* pokemons, int posHojaDeViaje , int *cantidad_muerte){
	int *result = malloc(sizeof(int));
	int tamanio_archivo, instruccion;
	char* nombre_archivo;
	enviarInt(socket_mapa, ATRAPAR_POKEMON);
	enviarInt(socket_mapa, 2);
	enviarMensaje(socket_mapa, nombre_pokemon);

	switch (instruccion=recibirInt(socket_mapa, result, entrenador_log)) {
		case POKEMON_CONCEDIDO:
			tamanio_archivo = recibirInt(socket_mapa, result, entrenador_log);
			char* nombre_archivo = malloc(sizeof(char) * tamanio_archivo);
			recibirMensaje(socket_mapa, nombre_archivo, tamanio_archivo, entrenador_log);
			list_add(pokemons, nombre_archivo);
			//TODO COPIAR ARCHIVO.DAT A DIRECTORIO BILL
			char* dirBill = get_entrenador_directorio_bill(ruta_pokedex,nombre_entrendor);
			copiar_archivo(nombre_archivo, dirBill);
			log_info(entrenador_log, "%s",nombre_archivo);
			free(result);
			//free(nombre_archivo);
			break;
		case MUERTE:
			printf("Ha muerto en una batalla");
			*cantidad_muerte +=1;
			if (!entrenador->vidas){
				reiniciar_Hoja_De_Viaje(posHojaDeViaje);
			} else {
				close(socket_mapa);
				entrenador->vidas--;
				borrar_pokemons_de_un_mapa(pokemons);
				flag_reconexion = true;
			}
			free(result);
			break;
	}
}


int avanzar_hacia_pokenest(){
	int movimiento_x, movimiento_y;


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

	enviarInt(socket_mapa, OBJETIVO_CUMPLIDO);

	//Recibo la ruta de medalla
	tamanio_texto = recibirInt(socket_mapa, result, entrenador_log);
	ruta_medalla = malloc(sizeof(char) * tamanio_texto);
	recibirMensaje(socket_mapa, ruta_medalla, tamanio_texto, entrenador_log);
	//TODO COPIAR MEDALLA AL DIRECTORIO /ENTRENADOR/NOMBRE/MEDALLAS/
	char* dirMedalla = get_entrenador_directorio_medallas(ruta_pokedex, nombre_entrendor);
	copiar_archivo(ruta_medalla, dirMedalla);
	free(result);
	free(ruta_medalla);

}

void convertirseEnMaestroPokemon(time_t tiempo_total_Viaje, time_t tiempo_total_bloqueado, int cantidad_muerte){
	printf("Tiempo del Viaje:  %f s.\n" , tiempo_total_bloqueado);
	printf("Tiempo bloqueado en Pokenest:  %f s.\n" , tiempo_total_Viaje);
	printf("Cantidad de veces que murio:  %d \n" , cantidad_muerte);
}


void recorrer_hojaDeViaje(int posHojaDeViaje) {
//	int posHojaDeViaje = 0; //Indice para recorrer la Hoja de Viaje
	int posObjetivoPorMapa = 0; //Indice para recorrer los Objetivos por Mapa
	int estado = CONECTARSE_MAPA;
	char **objetivosPorMapa;
	int cantidad_muerte=0;
	time_t inicio_De_Viaje, inicio_bloqueado, fin_bloqueado, fin_De_Viaje, tiempo_total_bloqueado, total_tiempo_viaje;
	tiempo_total_bloqueado=0;
	inicio_De_Viaje=time(NULL);
	t_list* pokemons_atrapados = list_create();; // es una lista temporal en caso de que en el mapa actual tenga que borrar los archivos por muerte
	while (hojaDeViaje[posHojaDeViaje]!= NULL){

		if(estado == CONECTARSE_MAPA){
			socket_mapa = conectar_mapa(ruta_pokedex, hojaDeViaje[posHojaDeViaje]);
			if(socket_mapa == 0){
				perror("Ocurrio un error al intentarse conectar al Mapa.");
				exit(1);
			}
			handshake();
		}
		list_clean(pokemons_atrapados);
		estado = UBICACION_POKENEST;
		posObjetivoPorMapa = 0;
		objetivosPorMapa = get_entrenador_objetivos_por_mapa(metadata, hojaDeViaje[posHojaDeViaje]);

		while(estado != OBJETIVO_CUMPLIDO && !flag_reinicio && !flag_reconexion){
			switch(estado){
				case UBICACION_POKENEST:
					//solicitar_posicion_pokenest(metadata, hojaDeViaje[posHojaDeViaje], posHojaDeViaje);
					solicitar_posicion_pokenest(objetivosPorMapa[posObjetivoPorMapa]);
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
					inicio_bloqueado=time(NULL);
					capturar_pokemon(/*"P"*/objetivosPorMapa[posObjetivoPorMapa],pokemons_atrapados,posHojaDeViaje, &cantidad_muerte);
					fin_bloqueado = time(NULL);
					tiempo_total_bloqueado+=difftime(fin_bloqueado,inicio_bloqueado);
					if (objetivoCumplido(/*0,0*/posHojaDeViaje,posObjetivoPorMapa)){
						estado = OBJETIVO_CUMPLIDO;
					} else {
						//posObjetivoPorMapa++;
						//posObjetivoPorMapa = 0;
						posObjetivoPorMapa++;
						estado = UBICACION_POKENEST;
					}
					break;
				default:
					log_error(entrenador_log, "Se ha producido un error al intentar realizar una accion del Entrenador.");
					break;
			}
		}

		if (flag_reinicio){
			estado = CONECTARSE_MAPA;
			posHojaDeViaje = 0;
			flag_reinicio = false;
			cantidad_muerte=0;
			tiempo_total_bloqueado=0;
			inicio_De_Viaje=0;
		}
		else {

		if (flag_reconexion){
			flag_reconexion = false;
			estado = CONECTARSE_MAPA;
		}
		else{
		//Espera la Medalla por Parte del Mapa
		terminarObjetivo();

		posHojaDeViaje++;

		if (hojaDeViaje[posHojaDeViaje] == NULL){
				printf("TE CONVERTISTE EN UN ENTRENADOR POKEMON!. \n");
				fin_De_Viaje = time(NULL);
				total_tiempo_viaje = difftime(fin_De_Viaje,inicio_De_Viaje);
				convertirseEnMaestroPokemon(total_tiempo_viaje,tiempo_total_bloqueado, cantidad_muerte);
				close(socket_mapa);
				free(pokemons_atrapados);
				int i=0;
				while (objetivosPorMapa[i]!=NULL ) {
					free(objetivosPorMapa[i]);
					i++;
				}
				free(objetivosPorMapa);
				for (i=0;i<list_size(pokemons_atrapados); i++){
					free(list_get(pokemons_atrapados,i));
				}
				free(pokemons_atrapados);

		} else {

			estado = CONECTARSE_MAPA;
		}

		}
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
		free(simbolo);
}

void init_datos_entrenador(){

	entrenador = malloc(sizeof(t_entrenador));
	entrenador -> vidas = get_entrenador_vidas(metadata);
	entrenador -> reintentos = get_entrenador_reintentos(metadata);

}

void reiniciar_Hoja_De_Viaje(int posHojaDeViaje){
	char c;

	printf("No te quedan más vidas para continuar con tu aventura POKEMON, querés reintentar?: (Y/N)\n");
	printf("La cantidad de reintentos hasta el momento es %d", entrenador->reintentos);

	switch (c = getchar()) {
	case 'Y':
	case 'y':
		entrenador->reintentos++;

		// Borrar medallas obtenidas;
		borrar_medallas();
		// Borrar pokemones obtenidos;
		borrar_pokemon();
		// Reiniciar hoja de viaje;
		close(socket_mapa);
		sleep(2); //PARA HACER PRUEBAS DE RECONEXION CON EL MAPA (SIN SLEEP TIRA ERROR )
		log_info(entrenador_log, "Reinicia el recorrido de ruta de viaje ");
		flag_reinicio = true;
		break;
	case 'N':
	case 'n':
		// Cerrar conexion, cerrar proceso, abandonar juego;
		printf("Abandonaste el juego, se cerrará la conexión y terminará el proceso");
		close(socket_mapa);
		flag_fin_prog = true;
		liberar_recursos();
		exit(1);
		break;
	default:
		break;
	}
}


void borrar_medallas(void){
	char* path_medalla = get_entrenador_directorio_medallas(ruta_pokedex,nombre_entrendor);
	DIR* dir_medalla = opendir(path_medalla);
	char *contenido = string_new();
	string_append(&contenido,".jpeg");
	borrar(dir_medalla,contenido,path_medalla);
	closedir(dir_medalla);
	free(contenido);

}

void liberar_recursos(){
	int i=0;
	while (hojaDeViaje[i]!=NULL ) {
		free(hojaDeViaje[i]);
		i++;
	}
	free(hojaDeViaje);

		log_destroy(entrenador_log);
		free(entrenador->nombre);
		free(entrenador);
		free(nombre_entrendor);
		free(ruta_pokedex);
		free(posicion_pokenest);
		free(posicion_mapa);
		free(entrenador_log);
		free(metadata);
}

void borrar_pokemon(void){
	char* path_pokemon = get_entrenador_directorio_bill(ruta_pokedex,nombre_entrendor);
	DIR* dir_bill = opendir(path_pokemon);
	char *contenido = string_new();
	string_append(&contenido,".dat");
	borrar(dir_bill,contenido,path_pokemon);
	closedir(dir_bill);
	free(contenido);

}

void borrar(DIR* deDirectorio, char* contenido, char* path_dir) {
	char* archivo = string_new();
	char* path_total = string_new();
	char* ret = string_new();
	int rem;
	int cant_archivos = 0; //variable para verificar la cantidad de archivos en el directorio
						   //durante debug
	struct dirent *ep = malloc(sizeof(struct dirent));

	memset(ep->d_name, '\0', sizeof(ep->d_name));

	if (deDirectorio != NULL) {
		while ((ep = readdir(deDirectorio)) != NULL) {
			if ((strcmp(ep->d_name, ".") != 0)
					&& (strcmp(ep->d_name, "..") != 0)) {
				cant_archivos++;
				string_append(&ret, strstr(ep->d_name, contenido));
				if (*ret != '\0') {
					string_append(&archivo, ep->d_name);
					string_append(&path_total, path_dir);
					string_append(&path_total, "/");
					string_append(&path_total, archivo);
					rem = remove(path_total);
					ret = strcpy(ret, "");
					archivo = strcpy(archivo, "");
					path_total = strcpy(path_total, "");
				}
			}
		}
	}
	free(archivo);
	free(path_total);
	free(ret);
}


void borrar_pokemons_de_un_mapa(t_list * pokemons){
	char* path_pokemon = get_entrenador_directorio_bill(ruta_pokedex,nombre_entrendor);
	int i;
	for (i=0;i<list_size(pokemons);i++) {
		borrar_del_mapa(path_pokemon, list_get(pokemons,i));
	}
	free(path_pokemon);
}

void borrar_del_mapa(char* directorio, char* archivo) {
	char* path_total = string_new();
	int rem;

	string_append(&path_total, directorio);
	string_append(&path_total, "/");
	string_append(&path_total, archivo);
	rem = remove(path_total);

	archivo = strcpy(archivo, "");
	path_total = strcpy(path_total, "");

	free(path_total);
}



void copiar_archivo(char* path_from, char* path_to){
	char* path_src = string_new();
	char* path_dst = string_new();
	char* mapArchSrc;
	char* mapArchDst;
	struct stat buf;
	char* nombre_archivo;
	char** path_split;
	char i = 0;
	string_append(&path_src,path_from);
	path_split = string_split(path_from,"/");

	while(*(path_split + i) != NULL){
		nombre_archivo = *(path_split + i);
		i++;
	}

	//El path destino seria /Entrenadores/[nombre entrenador]/directorio de bill
	string_append(&path_dst,path_to);
	string_append(&path_dst,nombre_archivo);
	int fd_src = open(path_src, O_RDWR);
	int fd_dst = open(path_dst, O_CREAT | O_RDWR, S_IRWXU);
	stat(path_src,&buf);
	int tam = buf.st_size;
	ftruncate(fd_dst,tam);
	mapArchSrc = (char*)mmap(0, tam, PROT_READ | PROT_WRITE, MAP_SHARED, fd_src, 0);
	mapArchDst = (char*)mmap(0, tam, PROT_WRITE, MAP_SHARED, fd_dst, 0);
	memcpy(mapArchDst, mapArchSrc, tam);
	munmap(mapArchSrc, tam);
	munmap(mapArchDst, tam);
	free(path_src);
	free(path_dst);

}
