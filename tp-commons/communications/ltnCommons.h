#include <unistd.h>
#include <sys/types.h>
#include <commons/config.h>
#include <commons/log.h>
#include <pthread.h>
#include <semaphore.h>


// Protocolo de comunicacion
#define GETATTR 0
#define READDIR 1
#define READ 2
#define MKDIR 3
#define CREATE 4
#define WRITE 5
#define UNLINK 6
#define TRUNCATE 7
#define RMDIR 8

#define FIN_GETATTR 0
#define FIN_READDIR 1
#define FIN_READ 2
#define FIN_MKDIR 3
#define FIN_CREATE 4
#define FIN_WRITE 5
#define FIN_UNLINK 6
#define FIN_TRUNCATE 7
#define FIN_RMDIR 8

#define ARCHIVO_NO_ENCONTRADO -2
#define ARCHIVO_ENCONTRADO 0
#define ST_MODE_CODE_ES_ARCHIVO 0
#define ST_MODE_CODE_ES_DIRECTORIO 1

// Para READDIR
#define CONTINUAR_READDIR -2
#define FINALIZAR_READDIR -3

// Para MKDIR y CREATE
#define MKDIR_DIRECTORIO 2
#define CREATE_FILE 1

typedef struct {
	int socketNumber;
	struct addrinfo *serverinfo;
} ltn_sock_addinfo;

char * intToString(int);
t_config * getConfig();

typedef struct {
	fd_set masterSet;
	fd_set readFileDescriptorSet;
	fd_set writeFileDescriptorSet;
	int maxFileDescriptorNumber;
	int listenerSocket;
} ltn_fd_sets;

int enviarMensaje(int clienteFd, char* mensaje);
int enviarInt(int clientFd, int mensaje);
int recibirMensaje(int fd, char * buf, int tamanio, t_log * myLog);
int recibirInt(int fd , int * result, t_log * myLog);
int enviarBytes(int clienteFd, void * datos, int tamanio);
int recibirBytes(int fd, void * buf, int tamanio, t_log * myLog);


int recibirBytesRaw(int fd, void * buf, int tamanio, t_log * myLog);
int enviarOffT(int clientFd, off_t mensaje);
off_t recibirOffT(int fd, int * result, t_log * myLog);

t_log * CreacionLogWithLevel(char * logName, char * programName, char * logLevel);
t_log * CreacionLog(char * logName, char * programName);

t_config* creacion_config();
t_config* creacion_config_with_path(char *);

void * myMalloc(size_t size, t_log * myLog);
void * myMalloc_int(char * nombreDeVariable, t_log * myLog);
void * myMalloc_char(int tamanio, char * nombreDeVariable, t_log * myLog);
void * myMalloc_ltn_sock_addinfo(char * nombreDeVariable, t_log * myLog);
void myFree(void * ptr, char * nombreDeVariable, t_log * myLog);
void myFreeSplitPath(char ** arr, char * nombreDeVariable, t_log * myLog);

void * myMalloc_int_array(int tamanio, char * nombreDeVariable, t_log * myLog);
void * myMalloc_mutex_t(int tamanio, char * nombreDeVariable, t_log * myLog);

