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

int recibir_datos(int socket_fd, void *dest, uint32_t size) {
	return recv(socket_fd, dest, size, 0);
}

int enviar_datos(int socket_fd, void *source, uint32_t size) {
	return send(socket_fd, source, size, 0);
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
void agregar_entero_sin_signo_a_paquete(t_paquete* paquete, uint32_t numero)
{
    paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + sizeof(uint32_t));
    memcpy(paquete->buffer->stream + paquete->buffer->size, &numero, sizeof(uint32_t));
    paquete->buffer->size += sizeof(uint32_t);
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

void agregar_lista_de_cadenas_a_paquete(t_paquete* paquete, t_list* palabras)
{
	int cant_elementos = list_size(palabras);
	agregar_entero_a_paquete(paquete,cant_elementos);

	for(int i=0; i<cant_elementos; i++)
	{
		char* palabra = list_get(palabras, i);
		agregar_cadena_a_paquete(paquete, palabra);		
	}
}

void agregar_puntero_a_paquete(t_paquete* paquete, void* puntero, uint32_t tamanio_puntero)
{
    paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + sizeof(uint32_t) + tamanio_puntero);

    memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio_puntero, sizeof(uint32_t));
    paquete->buffer->size += sizeof(uint32_t);

    if (tamanio_puntero > 0) {
        memcpy(paquete->buffer->stream + paquete->buffer->size, puntero, tamanio_puntero);
        paquete->buffer->size += tamanio_puntero;
    }
}

void agregar_bytes_a_paquete(t_paquete* paquete, void* bytes, uint32_t tamanio_bytes)
{
    // vemo que el puntero y tamaño sea correcto
    if (bytes == NULL || tamanio_bytes == 0) {
        printf("Error: Puntero de bytes nulo o tamaño de bytes 0\n");
        return;
    }

    // Realloc para ajustar el tamaño del stream
    paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + sizeof(uint32_t) + tamanio_bytes);

    // Copiar el tamaño de los bytes al stream
    memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio_bytes, sizeof(uint32_t));
    paquete->buffer->size += sizeof(uint32_t);

    // Copiar los bytes al stream
    memcpy(paquete->buffer->stream + paquete->buffer->size, bytes, tamanio_bytes);
    paquete->buffer->size += tamanio_bytes;
}


char* sacar_cadena_de_paquete(void** stream)
{
	int tamanio_cadena = -1;
	char* cadena = NULL;

	memcpy(&tamanio_cadena, *stream, sizeof(int));
	if(tamanio_cadena<0) printf("cadena de largo negativo falla en sacar cadena paquete  \n");
	if(tamanio_cadena==0) printf("Me llego tamaño de cadena 0  \n");
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

uint32_t sacar_entero_sin_signo_de_paquete(void** stream)
{
    uint32_t numero = 0; // Inicializado a 0 en lugar de -1
    memcpy(&numero, *stream, sizeof(uint32_t));
    *stream += sizeof(uint32_t);

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

t_list* sacar_lista_de_cadenas_de_paquete(void** stream) 
{
	t_list* varias_palabras =  list_create();

	int cant_elementos = sacar_entero_de_paquete(stream);

	for(int i=0; i<cant_elementos; i++)
	{
		char* cadena = sacar_cadena_de_paquete(stream);
		list_add(varias_palabras, cadena);
	}

	return varias_palabras;
}

void* sacar_puntero_de_paquete(void** stream)
{
    void* puntero = NULL;
    uint32_t tamanio_puntero = 0;

    memcpy(&tamanio_puntero, *stream, sizeof(uint32_t));
    *stream += sizeof(uint32_t);

    if (tamanio_puntero > 0) {
        puntero = malloc(tamanio_puntero);
        memcpy(puntero, *stream, tamanio_puntero);
        *stream += tamanio_puntero;
    }

    return puntero;
}

void* sacar_bytes_de_paquete(void** stream, uint32_t* tamanio_bytes)
{
    
    if (*stream == NULL) {
        printf("Error: Puntero de stream nulo\n");
        *tamanio_bytes = 0;
        return NULL;
    }

    // lee el tamaño de los bytes del stream
    memcpy(tamanio_bytes, *stream, sizeof(uint32_t));
    *stream += sizeof(uint32_t);

    // se fija uwu si hay bytes para leer
    if (*tamanio_bytes == 0) {
        return NULL;
    }

    // saca los bytes del stream
    void* bytes = malloc(*tamanio_bytes);
    memcpy(bytes, *stream, *tamanio_bytes);
    *stream += *tamanio_bytes;

    return bytes;
}

void eliminar_paquete(t_paquete* paquete)
{
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}

//================================================== SERIALIZACION =====================================================================
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

//============================================== Liberar memoria ===================================================
void free_array (char ** array){
	int tamanio = string_array_size(array);
	for (int i = 0; i<tamanio; i++) free(array[i]);
	free(array);
}


/*/================================================== BUFFER =====================================================================
void* recibir_buffer(int* size, int socket_cliente) {
     void * buffer;

     recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
     buffer = malloc(*size);
     recv(socket_cliente, buffer, *size, MSG_WAITALL);

     return buffer;
 }

void agregar_a_buffer(t_buffer *buffer, void *src, int size) {
	buffer->stream = realloc(buffer->stream, buffer->stream_size + size);
	memcpy(buffer->stream + buffer->stream_size, src, size);
	buffer->stream_size+=size;
}

t_buffer *inicializar_buffer_con_parametros(uint32_t size, void *stream) {
	t_buffer *buffer = (t_buffer *)malloc(sizeof(t_buffer));
	buffer->stream_size = size;
	buffer->stream = stream;
	return buffer;
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

    void *a_enviar = serializar_paquete(paquete,bytes);

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
*/


