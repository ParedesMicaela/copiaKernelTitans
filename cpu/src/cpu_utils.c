#include "cpu_utils.h"

//======================== Variables Globales ========================================================================
pthread_mutex_t mutex_interrupcion;
int interrupcion;
int tam_pagina;
t_config *config;
t_log *cpu_logger;
arch_config config_valores_cpu;
uint32_t AX;
uint32_t BX;
uint32_t CX;
uint32_t DX;
char *instruccion;

//======================= Funciones Internas ==============================================================================
static void enviar_handshake(int socket_cliente_memoria);
static void recibir_handshake(int socket_cliente_memoria);
static void setear_registro(char *registro, int valor);
static int sumar_registros(char *registro_destino, char *registro_origen);
static int restar_registros(char *registro_destino, char *registro_origen);
static int buscar_registro(char *registros);
static int tipo_inst(char *instruccion);
static void devolver_contexto_ejecucion(int socket_cliente, t_contexto_ejecucion *contexto_ejecucion, char *motivo);
static void enviar_contexto(int socket_cliente, t_contexto_ejecucion *contexto_ejecucion, char *motivo);
static void pedir_instruccion(int socket_cliente_memoria,int posicion);
static void recibir_instruccion(int socket_cliente_memoria);


//================================================== Configuracion =====================================================================

// funcion para levantar el archivo de configuracion de cfg y ponerlo en nuestro stuct de cpu
void cargar_configuracion(char *path)
{
    config = config_create(path);

    if (config == NULL)
    {
        perror("Archivo de configuracion de cpu no encontrado \n");
        abort();
    }

    config_valores_cpu.ip_cpu = config_get_string_value(config, "IP_CPU");
    config_valores_cpu.ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    config_valores_cpu.puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    config_valores_cpu.puerto_escucha_dispatch = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
    config_valores_cpu.puerto_escucha_interrupt = config_get_string_value(config, "PUERTO_ESCUCHA_INTERRUPT");

    log_info(cpu_logger, "Configuracion de CPU cargada");
}

//================================================== Handshake =====================================================================
void realizar_handshake(int socket_cliente_memoria)
{
    enviar_handshake(socket_cliente_memoria);
    log_info(cpu_logger, "Handshake enviado a memoria \n");
    recibir_handshake(socket_cliente_memoria);
}

static void enviar_handshake(int socket_cliente_memoria)
{
    t_paquete *paquete = crear_paquete(HANDSHAKE);
    agregar_entero_a_paquete(paquete, 1);
    enviar_paquete(paquete, socket_cliente_memoria);
}

static void recibir_handshake(int socket_cliente_memoria)
{
    t_paquete* paquete = recibir_paquete(socket_cliente_memoria);
    void* stream = paquete->buffer->stream;
    if (paquete->codigo_operacion == HANDSHAKE)
    {
        tam_pagina = sacar_entero_de_paquete(&stream);
        log_info(cpu_logger, "Tamanio de pagina %d para realizar handshake :)", tam_pagina);
    }
    else
    {
        perror("No me enviaste el tam_pagina :( \n");
        abort();
    }
}

//================================================== Instrucciones =====================================================================

static void recibir_instruccion(int socket_cliente_memoria)
{
    t_paquete *paquete = recibir_paquete(socket_cliente_memoria);
    void *stream = paquete->buffer->stream;

    if (paquete->codigo_operacion == INSTRUCCIONES)
    {
        instruccion = sacar_cadena_de_paquete(&stream);
    }
    else
    {
        perror("No me enviaste las instrucciones\n");
        abort();
    }
}

static void pedir_instruccion(int socket_cliente_memoria,int posicion)
{
    t_paquete *paquete = crear_paquete(MANDAR_INSTRUCCIONES);
    agregar_entero_a_paquete(paquete,posicion);
    enviar_paquete(paquete, socket_cliente_memoria);
    eliminar_paquete(paquete);
}

//================================================== Dispatch =====================================================================

// por aca el kernel nos va a mandar el pcb y es el canal importante donde recibimos y mandamos el contexto ejecucion
void atender_dispatch(int socket_cliente_dispatch, int socket_cliente_memoria)
{
    log_info(cpu_logger, "Espero recibir paquete");
    t_paquete *paquete = recibir_paquete(socket_cliente_dispatch);
    void *stream = paquete->buffer->stream;
    log_info(cpu_logger, "Ya recibi paquete");
    log_info(cpu_logger, "recibi %d de %d\n",paquete->codigo_operacion,socket_cliente_dispatch);
    
    t_contexto_ejecucion *contexto_ejecucion = malloc(sizeof(t_contexto_ejecucion));

    // el kernel nos va a pasar el pcb al momento de poner a ejecutar un proceso
    if (paquete->codigo_operacion == PCB)
    {
        contexto_ejecucion->pid = sacar_entero_de_paquete(&stream);
        contexto_ejecucion->program_counter = sacar_entero_de_paquete(&stream);
        contexto_ejecucion->prioridad = sacar_entero_de_paquete(&stream);
        contexto_ejecucion->registros_cpu.AX = sacar_entero_sin_signo_de_paquete(&stream);
        contexto_ejecucion->registros_cpu.BX = sacar_entero_sin_signo_de_paquete(&stream);
        contexto_ejecucion->registros_cpu.CX = sacar_entero_sin_signo_de_paquete(&stream);
        contexto_ejecucion->registros_cpu.DX = sacar_entero_sin_signo_de_paquete(&stream);

        log_info(cpu_logger, "Recibi un PCB del Kernel :)");

        
        // iniciamos el procedimiento para procesar cualquier instruccion
        ciclo_de_instruccion(socket_cliente_dispatch, socket_cliente_memoria, contexto_ejecucion);
    }
    else
    {
        // los sleep son como para que parezca que la cpu en serio esta pensando y tarda un poquito
        sleep(3);
        perror("No se recibio correctamente el PCB");
    }
}

//================================================== Ciclo de Instruccion =====================================================================

// le pasamos el socket del kernel y el socket de la memoria, porque son los modulos con los que voy a tener que relacionarme
void ciclo_de_instruccion(int socket_cliente_dispatch, int socket_cliente_memoria, t_contexto_ejecucion *contexto_ejecucion)
{
    bool seguir_ejecutando = true;

    while (seguir_ejecutando) // definir con cuentas voy, definir con program counter
    {

        // estos son los registros de la cpu que ya inicializamos arriba y almacenan valores enteros no signados de 4 bytes
        log_info(cpu_logger, "AX = %d BX = %d CX = %d DX = %d", AX, BX, CX, DX);

        //=============================================== FETCH =================================================================
        
        //le mando el program pointer a la memoria para que me pase la instruccion a la que apunta
        pedir_instruccion(socket_cliente_memoria, contexto_ejecucion->program_counter);
        log_info(cpu_logger,"Pidiendo instruccion a memoria\n");

        //una vez que la recibo de memoria, la guardo en la var global de arriba
        recibir_instruccion(socket_cliente_memoria);
        log_info(cpu_logger,"Recibi una instruccion de memoria\n");

     /*
        //=============================================== DECODE =================================================================
        // vemos si la instruccion requiere de una traducción de dirección lógica a dirección física
        
        if(requiere_traduccion(instruccion))
        {
            log_info(cpu_logger,"%s requiere traduccion", instruccion);
            //traducir_a_direccion_fisica(instruccion);
        }
        
*/
        //=============================================== EXECUTE =================================================================

        // toda esta parte la usamos para trabajar con registros (sumar,restar,poner)
        char **datos = string_split(instruccion, " ");
        char *registro = NULL;
        char *registro_destino = NULL;
        char *registro_origen = NULL;
        int valor = -1;

        switch (tipo_inst(datos[0]))
        {
        case (SET):
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s - %s", contexto_ejecucion->pid, datos[0], datos[1], datos[2]);
            registro = datos[1];
            valor = atoi(datos[2]);
            setear_registro(registro, valor);
            //devolver_contexto_ejecucion(socket_cliente_dispatch, contexto_ejecucion, "SET");
            contexto_ejecucion->program_counter += 1;
            break;

        case (SUM):
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s - %s", contexto_ejecucion->pid, datos[0], datos[1], datos[2]);
            registro_destino = datos[1];
            registro_origen = datos[2];
            valor = sumar_registros(registro_destino, registro_origen);
            setear_registro(registro_destino, valor);
            //devolver_contexto_ejecucion(socket_cliente_dispatch, contexto_ejecucion, "SUM");
            contexto_ejecucion->program_counter += 1;
            break;

        case (SUB):
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s - %s", contexto_ejecucion->pid, datos[0], datos[1], datos[2]);
            registro_destino = datos[1];
            registro_origen = datos[2];
            valor = restar_registros(registro_destino, registro_origen);
            setear_registro(registro_destino, valor);
            //devolver_contexto_ejecucion(socket_cliente_dispatch, contexto_ejecucion, "SUB");
            contexto_ejecucion->program_counter += 1;
            break;

        case (INSTRUCCION_EXIT):
            log_info(cpu_logger, "PID: %d - Ejecutando: %s", contexto_ejecucion->pid, datos[0]);
            devolver_contexto_ejecucion(socket_cliente_dispatch, contexto_ejecucion, "exit");
            // eliminar_todas_las_entradas(contexto_ejecucion->pid);
            seguir_ejecutando = false;
            break;
        } 
    }
}

//================================================== Interrupt =====================================================================

// este canal se va a usar para mensajes de interrupcion
void atender_interrupt(void *cliente)
{
    int conexion = *(int *)cliente;

    while (1)
    {
        t_paquete *paquete = recibir_paquete(conexion);
        log_info(cpu_logger, "Recibi un aviso por interrupt del kernel");
        //void *stream = paquete->buffer->stream; no la usamos todavia

        if (paquete->codigo_operacion == DESALOJO)
        {
            // esto despues lo usamos en el check interrupt para saber que si hay una interrupcion, devolvemos el contexto de ejecucion
            // int entero = sacar_entero_de_paquete(&stream);

            // lo tenemos que poner en un semaforo para que nada lo modifique mientras le sumo 1, cosa que despues no se pierda que
            // efectivamente hubo un interrupcion
            pthread_mutex_lock(&mutex_interrupcion);
            interrupcion += 1;
            pthread_mutex_unlock(&mutex_interrupcion);
        }
        else
        {
            printf("No recibi una interrupcion\n");
        }
    }
}



//================================================== REGISTROS =====================================================================
static void setear_registro(char *registros, int valor)
{
    if (string_equals_ignore_case(registros, "AX"))
        AX = valor;

    if (string_equals_ignore_case(registros, "BX"))
        BX = valor;

    if (string_equals_ignore_case(registros, "CX"))
        CX = valor;

    if (string_equals_ignore_case(registros, "DX"))
        DX = valor;
}

static int sumar_registros(char *registro_destino, char *registro_origen)
{
    int valor1 = buscar_registro(registro_destino);
    int valor2 = buscar_registro(registro_origen);

    int suma = valor1 + valor2;

    return suma;
}

static int restar_registros(char *registro_destino, char *registro_origen)
{
    int valor1 = buscar_registro(registro_destino);
    int valor2 = buscar_registro(registro_origen);

    int resta = valor1 - valor2;

    return resta;
}

static int buscar_registro(char *registros)
{
    int valor = -1;

    if (string_equals_ignore_case(registros, "AX"))
        valor = AX;

    if (string_equals_ignore_case(registros, "BX"))
        valor = BX;

    if (string_equals_ignore_case(registros, "CX"))
        valor = CX;

    if (string_equals_ignore_case(registros, "DX"))
        valor = DX;

    return valor;
}

static int tipo_inst(char *instruccion)
{
    int numero = -1;

    if (string_equals_ignore_case(instruccion, "SET"))
        numero = SET;

    if (string_equals_ignore_case(instruccion, "SUM"))
        numero = SUM;

    if (string_equals_ignore_case(instruccion, "SUB"))
        numero = SUB;

    if (string_equals_ignore_case(instruccion, "EXIT"))
        numero = INSTRUCCION_EXIT;

    return numero;
}

//================================================== CONTEXTO =====================================================================

// vamos a devolver el contexto al kernel en las instrucciones exit, sleep
static void devolver_contexto_ejecucion(int socket_cliente, t_contexto_ejecucion *contexto_ejecucion, char *motivo)
{
    // aca nosotros agregamos las modificaciones de los registros
    (contexto_ejecucion->registros_cpu.AX) = AX;
    (contexto_ejecucion->registros_cpu.BX) = BX;
    (contexto_ejecucion->registros_cpu.CX) = CX;
    (contexto_ejecucion->registros_cpu.DX) = DX;
    enviar_contexto(socket_cliente, contexto_ejecucion, motivo);
    log_info(cpu_logger, "devolvi el contexo ejecucion al kernel por motivo de: %s \n", motivo);
}

static void enviar_contexto(int socket_cliente, t_contexto_ejecucion *contexto_ejecucion, char *motivo)
{
    t_paquete *paquete = crear_paquete(PCB);

    // le mandamos esto porque creo que es lo unico que se cambia pero vemos
    agregar_entero_a_paquete(paquete, contexto_ejecucion->program_counter);
    agregar_entero_sin_signo_a_paquete(paquete, contexto_ejecucion->registros_cpu.AX);
    agregar_entero_sin_signo_a_paquete(paquete, contexto_ejecucion->registros_cpu.BX);
    agregar_entero_sin_signo_a_paquete(paquete, contexto_ejecucion->registros_cpu.CX);
    agregar_entero_sin_signo_a_paquete(paquete, contexto_ejecucion->registros_cpu.DX);
    agregar_cadena_a_paquete(paquete, motivo);
    // agregar_entero_a_paquete(paquete, contexto_ejecucion->hay_que_bloquear);

    enviar_paquete(paquete, socket_cliente);
}
