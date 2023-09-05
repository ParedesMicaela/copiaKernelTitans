#ifndef SOCKET_H_
#define SOCKET_H_

#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <commons/log.h>
#include "operaciones.h"

int esperar_cliente(t_log* logger, const char* nombre, int socket_servidor);
int iniciar_servidor(t_log* logger, const char* nombre, char* ip, char* puerto);
int crear_conexion(t_log* logger, const char* nombre_server, char* ip, char* puerto);
void liberar_conexion(int* socket_cliente);

#endif


