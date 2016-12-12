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
int posHojaDeViaje = 0;

//Nombre del entrenador
char *nombre_entrendor;
char* dirBill;
char* dirMedalla;

//Estructuras para el manejo de la posicion
t_posicion_pokenest *posicion_pokenest;
t_posicion_mapa *posicion_mapa;

pthread_mutex_t mutex_archivo;


// Funciones utilitarias //
//void init_datos_entrenador(void);
void borrar(DIR*, char*, char*);
void borrar_del_mapa(char*, char*);
void copiar_archivo(char*, char*);


int main(int argc, char **argv) {


	flag_fin_prog = false;
	flag_reinicio = false;
	flag_reconexion = false;



	if (argv[1] == NULL || argv[2] == NULL){
		perror("Se necesita el nombre del entrenador y la ruta de la Pokedex!");
		exit(1);
	}
	char *log_level = "INFO";
	entrenador_log = CreacionLogWithLevel(log_nombre, programa_nombre, log_level);
	log_info(entrenador_log, "Se ha creador el Log para el Entrenador.");

	nombre_entrendor = argv[1];
	ruta_pokedex = argv[2];

	posicion_pokenest = inicializar_posicion_pokenest();
	posicion_mapa =inicializar_posicion_entrenador();
	// Obtiene el archivo de metadata del entrenador.
	metadata = get_entrenador_metadata(ruta_pokedex, nombre_entrendor);
	init_datos_entrenador();
	dirBill = get_entrenador_directorio_bill(ruta_pokedex,nombre_entrendor);
	dirMedalla = get_entrenador_directorio_medallas(ruta_pokedex, nombre_entrendor);

	//Obtiene hoja de viaje del archivo metadata del entrenador
	hojaDeViaje = get_entrenador_hoja_de_viaje(metadata);



	//Manejo de System Calls
	signal(SIGTERM, system_call_catch);
	signal(SIGUSR1, system_call_catch);
	//signal(SIGKILL,system_call_catch);

	//Comieza el viaje del Entreandor
	recorrer_hojaDeViaje(posHojaDeViaje);

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
		borrar_medallas();
		log_info(entrenador_log, "Se borraron las medallas para finalizar el programa ");
		// Borrar pokemones obtenidos;
		borrar_pokemon();
		log_info(entrenador_log, "Se borraron los pokemons para finalizar el programa ");
		break;

	default:
		break;
	}
}



int conectar_mapa(char* ruta_pokedex, char *mapa){
	ltn_sock_addinfo *ltn_cliente_entrenador;
	t_config * metadata_mapa = (t_config*)get_mapa_metadata(ruta_pokedex , mapa);
	int puerto_del_mapa = (int)get_mapa_puerto(metadata_mapa);
	char *mapa_puerto = string_itoa(puerto_del_mapa);
	char *mapa_ip = (char*)get_mapa_ip( metadata_mapa);
	ltn_cliente_entrenador = createClientSocket(mapa_ip, mapa_puerto);
	int socket_mapa = doConnect(ltn_cliente_entrenador);

	free(mapa_puerto);
	config_destroy(metadata_mapa);


	return socket_mapa;
}

void solicitar_posicion_pokenest(char *pokemon){
	int *result = malloc(sizeof(int));
	int posicion_x, posicion_y;
	enviarInt(socket_mapa, UBICACION_POKENEST);
	log_info(entrenador_log,  "ENVIE UBICACION_POKENEST,instruccion: %d", UBICACION_POKENEST);
	//enviarInt(socket_mapa, (sizeof(char) * 2));
	enviarInt(socket_mapa, string_length(pokemon));
	//log_info(entrenador_log , "ENVIE  2");
	enviarMensaje(socket_mapa,pokemon);
	log_info(entrenador_log, "Coordenadas de la pokenest %s", pokemon);
	posicion_x = recibirInt(socket_mapa, result, entrenador_log);
	log_info(entrenador_log,"%d",posicion_x);
	posicion_y = recibirInt(socket_mapa, result, entrenador_log);
	log_info(entrenador_log,"%d",posicion_y);
	//Actualizo la posicion del proximo pokenest
	actualizar_posicion_pokenest(posicion_pokenest, posicion_x, posicion_y);
	free(result);
}


void capturar_pokemon(char *nombre_pokemon, t_list* pokemons, int posHojaDeViaje , int *cantidad_muerte, int *cantidad_deadlocks){
	int *result = malloc(sizeof(int));
	int tamanio_mensaje, instruccion;
	char* mensaje1;
	char* mensaje2;
	enviarInt(socket_mapa, ATRAPAR_POKEMON);
	//enviarInt(socket_mapa, 2);
	enviarInt(socket_mapa, string_length(nombre_pokemon));
	enviarMensaje(socket_mapa, nombre_pokemon);

	instruccion = recibirInt(socket_mapa, result, entrenador_log);

	while (*result > 0 && instruccion != MUERTE && instruccion != POKEMON_CONCEDIDO ){
		tamanio_mensaje = recibirInt(socket_mapa, result,entrenador_log);
		mensaje1 = malloc(sizeof(char) * (tamanio_mensaje + 1));
		recibirMensaje(socket_mapa, mensaje1,tamanio_mensaje,entrenador_log);
		instruccion = recibirInt(socket_mapa, result, entrenador_log);
		free(mensaje1);
		*cantidad_deadlocks +=1 ;
	}

	switch (instruccion) {

		case POKEMON_CONCEDIDO:
			tamanio_mensaje = recibirInt(socket_mapa, result, entrenador_log);
			mensaje2 = malloc(sizeof(char) * (tamanio_mensaje + 1));
			recibirMensaje(socket_mapa, mensaje2, tamanio_mensaje, entrenador_log);
			list_add(pokemons, mensaje2);
			pthread_mutex_lock(&mutex_archivo);
			copiar_archivo(mensaje2, dirBill);
			pthread_mutex_unlock(&mutex_archivo);

			log_info(entrenador_log, "Copio archivo %s a directorio bill ", mensaje2 );

			free(result);
			//free(mensaje2); //20161211 - FM
			break;

		case MUERTE:
			printf("Has muerto en una batalla POKEMON!! \n");
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
		default :
			if (flag_reinicio != true ){
			log_info(entrenador_log, "No se recibio instruccion del Mapa, posible desconexion ");
			 //Borrar medallas obtenidas;
			borrar_medallas();
			log_info(entrenador_log, "Se borraron las medallas para finalizar el programa ");
			// Borrar pokemones obtenidos;
			borrar_pokemon();
			log_info(entrenador_log, "Se borraron los pokemons para finalizar el programa ");
			free(result);
			exit(1);
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
	enviarInt(socket_mapa, posicion_mapa->x);

	//Me muevo en Y
	movimiento_y = moverse_en_mapa_eje_y(posicion_mapa, posicion_pokenest);
	enviarInt(socket_mapa, posicion_mapa->y);
	return llego_a_pokenest(posicion_mapa, posicion_pokenest);
	//Valida si el entrenador no se tiene que mover más, si es asi se devuelve 1, de lo contrario 0
	/*if(movimiento_x == 0 && movimiento_y == 0){
		return 1;
	} else {
		return 0;
	}*/
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
	ruta_medalla = malloc(sizeof(char) * (tamanio_texto + 1));
	recibirMensaje(socket_mapa, ruta_medalla, tamanio_texto, entrenador_log);

	pthread_mutex_lock(&mutex_archivo);
	copiar_archivo(ruta_medalla, dirMedalla);
	log_info (entrenador_log, "Se copio %s al directorio medallas ", ruta_medalla);
	pthread_mutex_unlock(&mutex_archivo);
	free(result);
	free(ruta_medalla);

}

void convertirseEnMaestroPokemon(double tiempo_total_Viaje, double tiempo_total_bloqueado, int cantidad_muerte, int cantidad_deadlocks){
	printf("Tiempo del Viaje: %f s.\n" , tiempo_total_bloqueado);
	printf("Tiempo bloqueado en Pokenest: %f s.\n" , tiempo_total_Viaje);
	printf("Cantidad de veces que murio: %d \n" , cantidad_muerte);
	printf("Cantidad de batallas: %d \n" , cantidad_deadlocks);

}


void recorrer_hojaDeViaje(int posHojaDeViaje) {

	int posObjetivoPorMapa = 0; //Indice para recorrer los Objetivos por Mapa
	int estado = CONECTARSE_MAPA;
	char **objetivosPorMapa;
	int cantidad_muerte = 0;
	int cantidad_deadlocks = 0;
	int index = 0; // indice para recorrer y liberar char** objetivosPorMapa
	time_t inicio_De_Viaje, inicio_bloqueado, fin_bloqueado, fin_De_Viaje;
	double tiempo_total_bloqueado, total_tiempo_viaje;
	tiempo_total_bloqueado = 0;
	inicio_De_Viaje = time(NULL);
	t_list* pokemons_atrapados = list_create(); // es una lista temporal en caso de que en el mapa actual tenga que borrar los archivos por muerte
	while (hojaDeViaje[posHojaDeViaje] != NULL) {

		if (estado == CONECTARSE_MAPA) {
			log_info(entrenador_log, "Me quiero conectar al mapa: %s",
					hojaDeViaje[posHojaDeViaje]);
			socket_mapa = conectar_mapa(ruta_pokedex,
					hojaDeViaje[posHojaDeViaje]);
			if (socket_mapa == 0) {
				perror("Ocurrio un error al intentarse conectar al Mapa.");
				exit(1);
			}
			log_info(entrenador_log, "Me conecté con éxito al mapa: %s",
					hojaDeViaje[posHojaDeViaje]);
			handshake();
		}
		list_clean(pokemons_atrapados);
		estado = UBICACION_POKENEST;
		posObjetivoPorMapa = 0;
		objetivosPorMapa = get_entrenador_objetivos_por_mapa(metadata,
				hojaDeViaje[posHojaDeViaje]);
		log_info(entrenador_log, "Tengo que atrapar al pokemon: %s",
				objetivosPorMapa[posObjetivoPorMapa]);
		while (estado != OBJETIVO_CUMPLIDO && !flag_reinicio && !flag_reconexion) {
			switch (estado) {
			case UBICACION_POKENEST:
				log_info(entrenador_log,
						"Solicito las coordenadas de la pokenest de: %s",
						objetivosPorMapa[posObjetivoPorMapa]);
				solicitar_posicion_pokenest(
						objetivosPorMapa[posObjetivoPorMapa]);
				estado = AVANZAR_HACIA_POKENEST;
				break;
			case AVANZAR_HACIA_POKENEST:
				log_info(entrenador_log,
						"Solicito avanzar a la pokenest de: %s",
						objetivosPorMapa[posObjetivoPorMapa]);

				if (avanzar_hacia_pokenest() == 1) {
					estado = ATRAPAR_POKEMON;
				} else {
					estado = AVANZAR_HACIA_POKENEST;
				}
				break;
			case ATRAPAR_POKEMON:
				inicio_bloqueado = time(NULL);
				log_info(entrenador_log,
						"Voy a intentar atrapar al pokemon: %s",
						objetivosPorMapa[posObjetivoPorMapa]);
				capturar_pokemon(/*"P"*/objetivosPorMapa[posObjetivoPorMapa],
						pokemons_atrapados, posHojaDeViaje, &cantidad_muerte,
						&cantidad_deadlocks);
				fin_bloqueado = time(NULL);
				tiempo_total_bloqueado += difftime(fin_bloqueado,
						inicio_bloqueado);
//					if (objetivoCumplido(/*0,0*/posHojaDeViaje,posObjetivoPorMapa)){
//						estado = OBJETIVO_CUMPLIDO;
//					}
				if (objetivosPorMapa[posObjetivoPorMapa + 1] == NULL) {
					estado = OBJETIVO_CUMPLIDO;
				} else {
					posObjetivoPorMapa++;
					estado = UBICACION_POKENEST;
				}
				break;
			default:
				log_error(entrenador_log,
						"Se ha producido un error al intentar realizar una accion del Entrenador.");
				break;
			}
		}

		if (flag_reinicio) {
			log_info(entrenador_log, "Reinicia valores de variables");
			estado = CONECTARSE_MAPA;
			posHojaDeViaje = 0;
			flag_reinicio = false;
			cantidad_muerte = 0;
			cantidad_deadlocks = 0;
			tiempo_total_bloqueado = 0;
			inicio_De_Viaje = 0;
		} else {

			if (flag_reconexion) {
				flag_reconexion = false;
				estado = CONECTARSE_MAPA;
			} else {
				//Espera la Medalla por Parte del Mapa
				terminarObjetivo();

				posHojaDeViaje++;

				if (hojaDeViaje[posHojaDeViaje] == NULL) {
					printf("TE CONVERTISTE EN UN ENTRENADOR POKEMON!. \n");
					fin_De_Viaje = time(NULL);
					total_tiempo_viaje = difftime(fin_De_Viaje,
							inicio_De_Viaje);
					convertirseEnMaestroPokemon(total_tiempo_viaje,
							tiempo_total_bloqueado, cantidad_muerte,
							cantidad_deadlocks);
					// Borrar medallas obtenidas;
					//borrar_medallas();
					//log_info(entrenador_log, "Se borraron las medallas para finalizar la Hoja de Viaje ");
					// Borrar pokemones obtenidos;
					//borrar_pokemon();
					//log_info(entrenador_log, "Se borraron los pokemons para finalizar la Hoja de Viaje");
					close(socket_mapa);
					list_destroy(pokemons_atrapados);

				} else {

					estado = CONECTARSE_MAPA;
				}

			}
		}
	}
	while (*(objetivosPorMapa + index) != NULL) {
		free(*(objetivosPorMapa + index));
		index++;
	}
	free(objetivosPorMapa);
}
void handshake(){

		enviarInt(socket_mapa, SOY_ENTRENADOR);
		int size_nombre = (int) strlen(nombre_entrendor);
		//Envio el nombre del Entrenador
		enviarInt(socket_mapa, size_nombre);
		enviarMensaje(socket_mapa, nombre_entrendor);
		//Envio el caracter representante en el mapa
		//enviarInt(socket_mapa, 2);
		char * simbolo = get_entrenador_simbolo(metadata);
		enviarInt(socket_mapa, string_length(simbolo));
		enviarMensaje(socket_mapa, simbolo);
		ingresar_a_nuevo_mapa(posicion_mapa);

}

void init_datos_entrenador(){

	entrenador = malloc(sizeof(t_entrenador));
	entrenador -> vidas = get_entrenador_vidas(metadata);
	//entrenador -> reintentos = get_entrenador_reintentos(metadata);

}

void reiniciar_Hoja_De_Viaje(int posHojaDeViaje){
	char c;

	printf("No te quedan más vidas para continuar con tu aventura POKEMON, querés reintentar?: (Y/N)\n");
	//printf("La cantidad de reintentos hasta el momento es %d \n", entrenador->reintentos);

	switch (c = getchar()) {
	case 'Y':
	case 'y':
		entrenador->reintentos++;

		// Borrar medallas obtenidas;
		borrar_medallas();
		log_info(entrenador_log, "Se borraron las medallas para reiniciar la Hoja de Viaje ");
		// Borrar pokemones obtenidos;
		borrar_pokemon();
		log_info(entrenador_log, "Se borraron los pokemons para reiniciar la Hoja de Viaje  ");
		// Reiniciar hoja de viaje;
		close(socket_mapa);
		sleep(2); //PARA HACER PRUEBAS DE RECONEXION CON EL MAPA (SIN SLEEP TIRA ERROR )
		log_info(entrenador_log, "Reinicia el recorrido de ruta de viaje ");
		flag_reinicio = true;
		break;
	case 'N':
	case 'n':
		// Cerrar conexion, cerrar proceso, abandonar juego;
		printf("Abandonaste el juego, se cerrará la conexión y terminará el proceso\n");
		close(socket_mapa);
		flag_fin_prog = true;
		// Borrar medallas obtenidas;
		borrar_medallas();
		log_info(entrenador_log, "Se borraron las medallas para finalizar el programa ");
		// Borrar pokemones obtenidos;
		borrar_pokemon();
		log_info(entrenador_log, "Se borraron los pokemons para finalizar el programa ");
		liberar_recursos();
		exit(1);
		break;
	default:
		break;
	}
}


void borrar_medallas(void){
	//char* path_medalla = get_entrenador_directorio_medallas(ruta_pokedex,nombre_entrendor);
	DIR* dir_medalla = opendir(dirMedalla);
	char *contenido = string_new();
	string_append(&contenido,".jpg");
	pthread_mutex_lock(&mutex_archivo);
	borrar(dir_medalla,contenido,dirMedalla);
	pthread_mutex_unlock(&mutex_archivo);
	closedir(dir_medalla);
	free(contenido); //20161210 - FM
	//free(path_medalla);

}

void liberar_recursos() {
	int index = 0; // indice para recorrer y liberar char** hojaDeViaje
	log_destroy(entrenador_log);
	config_destroy(metadata);
	free(entrenador);
	free(dirBill);
	free(dirMedalla);
	while (*(hojaDeViaje + index) != NULL) {
		free(*(hojaDeViaje + index));
		index++;
	}
	free(hojaDeViaje); //20161207 - FM
	free(posicion_pokenest);
	free(posicion_mapa);

}

void borrar_pokemon(void){
	//char* path_pokemon = get_entrenador_directorio_bill(ruta_pokedex,nombre_entrendor);
	DIR* dir_bill = opendir(dirBill);
	char *contenido = string_new();
	string_append(&contenido,".dat");
	borrar(dir_bill,contenido,dirBill);
	closedir(dir_bill);
	free(contenido); //20161210 - FM

}

void borrar(DIR* deDirectorio, char* contenido, char* path_dir) {
	char* archivo = string_new();
	char* path_total = string_new();
	char* ret = string_new();
	int rem;
	int cant_archivos = 0; //variable para verificar la cantidad de archivos en el directorio
						   //durante debug
	struct dirent* ep;//= malloc(sizeof(struct dirent));

	//memset(ep->d_name, '\0', sizeof(ep->d_name));

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
	//free(contenido); //20161210 - FM
}


void borrar_pokemons_de_un_mapa(t_list * pokemons){
	//char* path_pokemon = get_entrenador_directorio_bill(ruta_pokedex,nombre_entrendor); //20161211 - FM
	int i;
	for (i=0;i<list_size(pokemons);i++) {
		//borrar_del_mapa(path_pokemon, list_get(pokemons,i)); //20161211 - FM
		borrar_del_mapa(dirBill, list_get(pokemons,i));
	}
	//free(path_pokemon); //20161211 - FM
}

void borrar_del_mapa(char* directorio, char* archivo) {
	char* path_total = string_new();
	char ** path_split; //20161211 - FM
	char* nombre_archivo; //20161211 - FM
	int rem;
	int i = 0; //20161211 - FM
	int index = 0; //20161211 - FM

	path_split = string_split(archivo, "/"); //20161211 - FM

	while (*(path_split + i) != NULL) { //20161211 - FM
		nombre_archivo = *(path_split + i);
		i++;
	}

	string_append(&path_total, directorio);
	string_append(&path_total, "/");
	string_append(&path_total, nombre_archivo);
	rem = remove(path_total);

	//archivo = strcpy(archivo, ""); //20161211 - FM
	path_total = strcpy(path_total, "");

	while (*(path_split + index) != NULL) { //20161211 - FM
		free(*(path_split + index));
		index++;
	}
	free(path_split); //20161211 - FM

	free(path_total);
}



void copiar_archivo(char* path_from, char* path_to) {
	char* path_src = string_new();
	char* path_dst = string_new();
	char* mapArchSrc;
	char* mapArchDst;
	struct stat buf;
	char* nombre_archivo;
	char** path_split;
	char i = 0;	//indice para recorrer char** path_split
	int index = 0; //indice para recorrer y liberar char** path_split
	string_append(&path_src, path_from);
	path_split = string_split(path_from, "/");

	while (*(path_split + i) != NULL) {
		nombre_archivo = *(path_split + i);
		i++;
	}

	//El path destino seria /Entrenadores/[nombre entrenador]/directorio de bill
	string_append(&path_dst, path_to);
	string_append(&path_dst, nombre_archivo);
	int fd_src = open(path_src, O_RDWR);
	int fd_dst = open(path_dst, O_CREAT | O_RDWR, S_IRWXU);
	stat(path_src, &buf);
	int tam = buf.st_size;
	ftruncate(fd_dst, tam);
	mapArchSrc = (char*) mmap(0, tam, PROT_READ | PROT_WRITE, MAP_SHARED,
			fd_src, 0);
	mapArchDst = (char*) mmap(0, tam, PROT_WRITE, MAP_SHARED, fd_dst, 0);
	memcpy(mapArchDst, mapArchSrc, tam);
	munmap(mapArchSrc, tam);
	munmap(mapArchDst, tam);
	free(path_src);
	free(path_dst);

	while (*(path_split + index) != NULL) {
		free(*(path_split + index));
		index++;
	}
	free(path_split);

}
