#include "socket.h"


int iniciar_servidor(t_log* logger, const char* nombre, char* ip, char* puerto) {
    int socket_servidor;
    struct addrinfo hints, *servinfo;

    // Inicializando hints
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags    = AI_PASSIVE;

    // Recibe los addrinfo
    getaddrinfo(ip, puerto, &hints, &servinfo);

    bool conecto = false;

    // Itera por cada addrinfo devuelto
    for (struct addrinfo *p = servinfo; p != NULL; p = p->ai_next) {
        socket_servidor = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        // Fallo crear socket
        if (socket_servidor == -1) {
            perror("fallo al crear socket para el server");
            exit(1);
            continue;
        }


        // Fallo bind
        if (bind(socket_servidor, p->ai_addr, p->ai_addrlen) == -1) {
            close(socket_servidor);
            perror("fallo el bind para el server");
            exit(1);
            continue;
        }

        conecto = true;
        break;
    }

    if (!conecto) {
        free(servinfo);
        return 0;
    }

    // Escuchando hasta SOMAXCONN conexiones simultaneas
    if ((listen(socket_servidor, SOMAXCONN)) == -1){
        perror("fallo el listen del server");
    }

    log_info(logger, "%s escuchando en: %s (%s)\n", nombre, ip, puerto);

    freeaddrinfo(servinfo);

    return socket_servidor;
}


/*
 * Esperar conexion de cliente en un server abierto
 */
int esperar_cliente(t_log* logger, const char* nombre, int socket_servidor) {
    struct sockaddr_in dir_cliente;
    socklen_t tam_direccion = sizeof(struct sockaddr_in);

    int socket_cliente = accept(socket_servidor, (void*) &dir_cliente, &tam_direccion);

    //int socket_cliente = accept(socket_servidor, NULL, NULL);
    log_info(logger, "%s: se conecto un cliente\n", nombre);

    return socket_cliente;
}


int crear_conexion(t_log* logger, const char* nombre_server, char* ip, char* puerto) {
    struct addrinfo hints, *servinfo;

    // Init de hints
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags    = AI_PASSIVE;

    // Recibe addrinfo
    getaddrinfo(ip, puerto, &hints, &servinfo);

    // Crea un socket con la informacion recibida
    int socket_cliente = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

    // Fallo crear socket
    if (socket_cliente == -1) {
        freeaddrinfo(servinfo);
        log_error(logger, "Error al crear el socket para %s: %s", ip, puerto);
        return 1;
    }

    // Error conectando
    int connect_cliente = connect(socket_cliente, servinfo->ai_addr, servinfo->ai_addrlen);
    if (connect_cliente == -1) {
        freeaddrinfo(servinfo);
        log_error(logger, "Error al conectar a %s\n", nombre_server);
        return 1;
     } else {
        log_info(logger, "Conectado a %s en IP:%s puerto: %s\n", nombre_server, ip, puerto);
     }
    freeaddrinfo(servinfo);
    return socket_cliente;
}

//cerrar conexion
void liberar_conexion(int* socket_cliente) {
	close(*socket_cliente);
    *socket_cliente = -1;
}
