#ifndef LOGCONFIG_H_
#define LOGCONFIG_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include "socket.h"

// FUNCIONES

t_log* iniciar_logger(char*, char*, int, t_log_level);
t_config* iniciar_config(char*);
char* obtener_de_config(t_config*, char*);
int obtener_int_de_config(t_config*, char*);
float obtener_float_de_config(t_config*, char*);
bool config_tiene_todas_las_propiedades(t_config*, char**);
void terminar_programa(int, t_log*, t_config*);


#endif /* LOGCONFIG_H_ */
