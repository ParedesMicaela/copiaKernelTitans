#include "operaciones.h"

//===================================================== OPERACIONES =======================================================================
int recibir_operacion(int socket_cliente)
{
	int cod_op;
	if(recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) > 0)
		return cod_op;
	else
	{
		close(socket_cliente);
		return -1;
	}
}

//===================================================== PAQUETES =============================================================================

t_paquete* crear_paquete(op_code codigo)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = codigo;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = 0;
	paquete->buffer->stream = NULL;
	return paquete;
}

t_paquete* recibir_paquete(int conexion)
{
    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->buffer = malloc(sizeof(t_buffer));
    if(recv(conexion, &(paquete->codigo_operacion),sizeof(paquete->codigo_operacion),MSG_WAITALL)==-1)
		{
			paquete->codigo_operacion = -1;
			printf("recibir paquete recibio un menos 1\n");
			return paquete;
		}
	if(paquete->codigo_operacion==FINALIZACION){
		return paquete;
	}
	else{
    	recv(conexion, &(paquete->buffer->size),sizeof(paquete->buffer->size),MSG_WAITALL);
    	paquete->buffer->stream = malloc(paquete->buffer->size);
    	recv(conexion, paquete->buffer->stream, paquete->buffer->size,MSG_WAITALL);

		return paquete;
	}
}

void* serializar_paquete(t_paquete* paquete, int bytes)
{
	void * magic = malloc(bytes);
	int desplazamiento = 0;

	memcpy(magic + desplazamiento, &(paquete->codigo_operacion), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
	desplazamiento+= paquete->buffer->size;

	return magic;
}
void enviar_paquete(t_paquete* paquete, int socket_cliente)
{
	int bytes = paquete->buffer->size + 2*sizeof(int);
	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
}

void agregar_entero_a_paquete(t_paquete* paquete,int numero)
{
	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + sizeof(int));
	memcpy(paquete->buffer->stream + paquete->buffer->size, &numero, sizeof(int));
	paquete->buffer->size += sizeof(int);
}

void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio)
{
	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio + sizeof(int));

	memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));
	memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), valor, tamanio);

	paquete->buffer->size += tamanio + sizeof(int);
}

void agregar_cadena_a_paquete(t_paquete* paquete, char* palabra)
{
	agregar_a_paquete(paquete, (void*)palabra, string_length(palabra) +1);
}

void agregar_array_cadenas_a_paquete(t_paquete* paquete, char** palabras)
{
	int cant_elementos = string_array_size(palabras);
	agregar_entero_a_paquete(paquete,cant_elementos);

	for(int i=0; i<cant_elementos; i++)
	{
		agregar_cadena_a_paquete(paquete, palabras[i]);
	}
}

char* sacar_cadena_de_paquete(void** stream)
{
	int tamanio_cadena = -1;
	char* cadena = NULL;

	memcpy(&tamanio_cadena, *stream, sizeof(int));
	if(tamanio_cadena<0) printf("cadena de largo negativo falla en sacar cadena paquete  \n");
	*stream += sizeof(int);

	cadena = malloc(tamanio_cadena);
	memcpy(cadena, *stream , tamanio_cadena);
	*stream += tamanio_cadena;

	return cadena;
}

int sacar_entero_de_paquete(void** stream)
{
	int numero = -1;
	memcpy(&numero, *stream, sizeof(int));
	*stream += sizeof(int);

	return numero;
}

char** sacar_array_cadenas_de_paquete(void** stream)
{
	char** varias_palabras =  string_array_new();

	int cant_elementos = sacar_entero_de_paquete(stream);

	for(int i=0; i<cant_elementos; i++)
	{
		string_array_push(&varias_palabras, sacar_cadena_de_paquete(stream));
	}

	return varias_palabras;
}

void eliminar_paquete(t_paquete* paquete)
{
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}

//================================================== MENSAJES =====================================================================

void enviar_mensaje(char *mensaje, int socket_cliente) //TP0
{
    t_paquete *paquete = malloc(sizeof(t_paquete));

    //paquete->codigo_operacion = MENSAJE;
    paquete->buffer = malloc(sizeof(t_buffer));
    paquete->buffer->stream_size = strlen(mensaje) + 1;
    paquete->buffer->stream = malloc(paquete->buffer->stream_size);
    memcpy(paquete->buffer->stream, mensaje, paquete->buffer->stream_size);

    int bytes = paquete->buffer->stream_size + 2 * sizeof(int);

    void *a_enviar = serializar_paquete_con_bytes(paquete,bytes);

    send(socket_cliente, a_enviar, bytes, 0);

    free(a_enviar);
    eliminar_paquete(paquete);
}

void recibir_mensaje(int socket_cliente,t_log* logger) { //TP0
    int size;
    char* buffer = recibir_buffer(&size, socket_cliente);
    log_info(logger, "Me llego el mensaje %s", buffer);
    free(buffer);
}



