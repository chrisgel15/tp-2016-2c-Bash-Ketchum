#include "mapa.h"

t_config * get_mapa_metadata(char * ruta_pokedex , char * nombre_mapa)
{
	char * path = string_new();
	string_append(&path, ruta_pokedex);
	string_append(&path, "/");
	string_append(&path, MAPA_FOLDER);
	string_append(&path, "/");
	string_append(&path, nombre_mapa);
	string_append(&path, "/");
	string_append(&path, NOMBRE_ARCHIVO_METADATA);

	return creacion_config_with_path(path);
}

int get_mapa_tiempo_deadlock(t_config * metadata)
{
	return config_get_int_value(metadata, TIEMPO_DEADLOCK);
}

int get_mapa_batalla_on_off(t_config * metadata)
{
	return config_get_int_value(metadata, BATALLA);
}

char * get_mapa_algoritmo(t_config * metadata)
{
	return config_get_string_value(metadata, ALGORITMO);
}

int get_mapa_quantum(t_config * metadata)
{
	return config_get_int_value(metadata, QUANTUM);
}

int get_mapa_retardo(t_config * metadata)
{
	return config_get_int_value(metadata, RETARDO);
}

char * get_mapa_ip(t_config * metadata)
{
	return config_get_string_value(metadata, IP);
}

int get_mapa_puerto(t_config * metadata)
{
	return config_get_int_value(metadata, PUERTO);
}

char ** get_lista_de_pokenest(t_config * metadata)
{
//	TODO return ARREGLO CON LOS NOMBRES DE TODOS LOS POKENEST DEL MAPA
}





