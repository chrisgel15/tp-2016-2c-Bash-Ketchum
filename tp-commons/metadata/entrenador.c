#include "entrenador.h"

t_config * get_entrenador_metadata(char * ruta_pokedex , char * nombre_entrenador)
{

	char * path = string_new();
	string_append(&path, ruta_pokedex);
	string_append(&path, "/");
	string_append(&path, ENTRENADOR_FOLDER);
	string_append(&path, "/");
	string_append(&path, nombre_entrenador);
	string_append(&path, "/");
	string_append(&path, NOMBRE_ARCHIVO_METADATA);

	return creacion_config_with_path(path);
}

char * get_entrenador_simbolo(t_config * metadata)
{
	return config_get_string_value(metadata, SIMBOLO_ENTRENADOR);
}

char * get_entrenador_nombre(t_config * metadata)
{
	return config_get_string_value(metadata, NOMBRE_ENTRENADOR);
}

char ** get_entrenador_hoja_de_viaje(t_config * metadata)
{
	return config_get_array_value(metadata, HOJA_DE_VIAJE);
}

char ** get_entrenador_objetivos_por_mapa(t_config * metadata, char * mapa)
{
	char** array_value;//20161206 - FM
	char * key = string_new();
	string_append(&key , "obj[");
	string_append(&key , mapa);
	string_append(&key , "]");

	array_value = config_get_array_value(metadata, key);//20161206 - FM
	free(key);

	return array_value;
}

int get_entrenador_vidas(t_config * metadata)
{
	return config_get_int_value(metadata, VIDAS);
}

int get_entrenador_reintentos(t_config * metadata)
{
	return config_get_int_value(metadata, REINTENTOS);
}

char *get_entrenador_directorio_bill(char *ruta_pokedex , char *nombre_entrenador){
	char * path = string_new();
	string_append(&path, ruta_pokedex);
	string_append(&path, "/");
	string_append(&path, ENTRENADOR_FOLDER);
	string_append(&path, "/");
	string_append(&path, nombre_entrenador);
	string_append(&path, "/");
	string_append(&path, DIRECTORIO_BILL);
	string_append(&path, "/");

	return path;
}

char *get_entrenador_directorio_medallas(char *ruta_pokedex , char *nombre_entrenador){
	char * path = string_new();
	string_append(&path, ruta_pokedex);
	string_append(&path, "/");
	string_append(&path, ENTRENADOR_FOLDER);
	string_append(&path, "/");
	string_append(&path, nombre_entrenador);
	string_append(&path, "/");
	string_append(&path, MEDALLAS);
	string_append(&path, "/");

	return path;
}

// ** Ejemplo de Uso! ** //

//t_config * metadata;
//metadata = get_entrenador_metadata(ruta_pokedex, nombre_entrendor);
//int cantidad_keys = config_keys_amount(metadata);
//char * nombre = get_entrenador_nombre(metadata);
//char * simbolo = get_entrenador_simbolo(metadata);
//char ** hojaDeViaje = get_entrenador_hoja_de_viaje(metadata);
//bool tieneCiudadPlateada = config_has_property(metadata , "obj[CiudadPlateada]");
//bool tieneCiudadVerde = config_has_property(metadata , "obj[CiudadVerde]");
//bool tienePuebloPaleta = config_has_property(metadata , "obj[PuebloPaleta]");
//char ** objetivo1 = get_entrenador_objetivos_por_mapa(metadata, (hojaDeViaje[0]));
//char ** objetivo2 = get_entrenador_objetivos_por_mapa(metadata, (hojaDeViaje[1]));
//char ** objetivo3 = get_entrenador_objetivos_por_mapa(metadata, (hojaDeViaje[2]));
//int vidas = get_entrenador_vidas(metadata);
//int reintentos = get_entrenador_reintentos(metadata);
