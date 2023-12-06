#include "socket.h"

//CLIENTE

int crear_conexion(char *ip, char *puerto)
{
    struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &server_info);

	int socketCliente = socket(server_info->ai_family,
								server_info->ai_socktype,
								server_info->ai_protocol);

	if (!connect(socketCliente, server_info->ai_addr, server_info->ai_addrlen))
	{
		freeaddrinfo(server_info);
		return socketCliente;
	}
	else
	{
		freeaddrinfo(server_info);
		return -1;
	}
}



void liberar_conexion(int socket_cliente) {
    close(socket_cliente);
}
//SERVIDOR

int iniciar_servidor(char *ip, char *puerto)
{

    int socketServidor;

	struct addrinfo hints, *servinfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(NULL, puerto, &hints, &servinfo);

	// Creamos el socket de escucha del servidor
	socketServidor = socket(servinfo->ai_family, servinfo->ai_socktype,servinfo->ai_protocol);
	int temp = 1;
	setsockopt(socketServidor, SOL_SOCKET, SO_REUSEADDR, &(temp), sizeof(int));

	// Asociamos el socket a un puerto
	bind(socketServidor, servinfo->ai_addr, servinfo->ai_addrlen);

	// Escuchamos las conexiones entrantes
	listen(socketServidor, SOMAXCONN);

	freeaddrinfo(servinfo);
	printf("Listo para escuchar a mi cliente");

	return socketServidor;
}

int esperar_cliente(int socket_servidor) {
    int socketClienteFD = accept(socket_servidor, NULL, NULL);

	return socketClienteFD;
}
