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
int tamanio_recursos;

//lo usamos para ver si es desalojo o finalizacion
int tipo_interrupcion = -1;

//======================= Funciones Internas ==============================================================================
static void enviar_handshake(int socket_cliente_memoria);
static void recibir_handshake(int socket_cliente_memoria);
static void setear_registro(char *registro, int valor);
static int sumar_registros(char *registro_destino, char *registro_origen);
static int restar_registros(char *registro_destino, char *registro_origen);
static int buscar_registro(char *registros);
static int tipo_inst(char *instruccion);
static void devolver_contexto_ejecucion(int socket_cliente, t_contexto_ejecucion *contexto_ejecucion, char *motivo,char* recurso, int tiempo);
static void enviar_contexto(int socket_cliente, t_contexto_ejecucion *contexto_ejecucion, char *motivo,char* recurso, int tiempo);
static void pedir_instruccion(int socket_cliente_memoria,int posicion, int pid);
static void recibir_instruccion(int socket_cliente_memoria);
static bool hay_interrupcion();
static void mostrar_valores (t_contexto_ejecucion* contexto);

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
    eliminar_paquete(paquete);
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
        log_error(cpu_logger,"No me enviaste el tam_pagina :( \n");
        abort();
    }
    eliminar_paquete(paquete);

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
        log_error(cpu_logger,"Falla al recibir las instrucciones\n");
        abort();
    }
    eliminar_paquete(paquete);
}

static void pedir_instruccion(int socket_cliente_memoria,int posicion, int pid)
{
    t_paquete *paquete = crear_paquete(MANDAR_INSTRUCCIONES);
    agregar_entero_a_paquete(paquete,posicion);
    agregar_entero_a_paquete(paquete,pid);
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
    
    t_contexto_ejecucion* contexto_ejecucion = malloc(sizeof(t_contexto_ejecucion));
    contexto_ejecucion->recursos_asignados = malloc(3 *sizeof(t_recursos_asignados));

    // el kernel nos va a pasar el pcb al momento de poner a ejecutar un proceso
    if (paquete->codigo_operacion == PCB)
    {
        log_info(cpu_logger, "Recibi un PCB del Kernel :)\n");

        contexto_ejecucion->pid = sacar_entero_de_paquete(&stream);
        contexto_ejecucion->program_counter = sacar_entero_de_paquete(&stream);
        contexto_ejecucion->prioridad = sacar_entero_de_paquete(&stream);
        contexto_ejecucion->registros_cpu.AX = sacar_entero_sin_signo_de_paquete(&stream);
        contexto_ejecucion->registros_cpu.BX = sacar_entero_sin_signo_de_paquete(&stream);
        contexto_ejecucion->registros_cpu.CX = sacar_entero_sin_signo_de_paquete(&stream);
        contexto_ejecucion->registros_cpu.DX = sacar_entero_sin_signo_de_paquete(&stream);

        // Iterar sobre cada recurso y recibir la información del paquete
        for (int i = 0; i < tamanio_recursos; ++i) {

            char* nombre = sacar_cadena_de_paquete(&stream);
            strcpy(contexto_ejecucion->recursos_asignados[i].nombre_recurso,nombre); 
            contexto_ejecucion->recursos_asignados[i].instancias_recurso = sacar_entero_de_paquete(&stream);
            free(nombre);
        }

        // iniciamos el procedimiento para procesar cualquier instruccion
        ciclo_de_instruccion(socket_cliente_dispatch, socket_cliente_memoria, contexto_ejecucion);
    }
    if(paquete->codigo_operacion != PCB)
    {
       perror("No se recibio correctamente el PCB");  
       abort();
    }

    free(contexto_ejecucion);
    eliminar_paquete(paquete);
}

//================================================== Ciclo de Instruccion =====================================================================

// le pasamos el socket del kernel y el socket de la memoria, porque son los modulos con los que voy a tener que relacionarme
void ciclo_de_instruccion(int socket_cliente_dispatch, int socket_cliente_memoria, t_contexto_ejecucion *contexto_ejecucion)
{
    bool seguir_ejecutando = true;
/*
    //implementación para el page_fault (checkpoint 4)
    //antes de seguir_ejecutando me fijo si hay page fault
    if (hay_page_fault()) 
    {
        log_info(cpu_logger, "Oh no hermano, tenemos Page Fault: PID %d - Número de página %d", contexto_ejecucion->pid, obtener_numero_pagina(instruccion));
        devolver_contexto_ejecucion(socket_cliente_dispatch, contexto_ejecucion, "page_fault", "", 0);
        //como tengo page fault notificó al kernel y no puedo seguir ejecutando el proceso hasta manejar el page fault
        seguir_ejecutando = false;
    }
*/
    while (seguir_ejecutando) // definir con cuentas voy, definir con program counter
    {

        //=============================================== FETCH =================================================================
        log_info(cpu_logger, "PID: %d - FETCH - Program Counter: %d", contexto_ejecucion->pid, contexto_ejecucion->program_counter);

        //le mando el program pointer a la memoria para que me pase la instruccion a la que apunta
        pedir_instruccion(socket_cliente_memoria, contexto_ejecucion->program_counter, contexto_ejecucion->pid);

        //una vez que la recibo de memoria, la guardo en la var global de arriba
        recibir_instruccion(socket_cliente_memoria);

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
        free(instruccion);
        char *registro = NULL;
        char *registro_destino = NULL;
        char *registro_origen = NULL;
        char *recurso = NULL;
        char* modo_apertura = NULL;
        char* nombre_archivo = NULL;
        uint32_t direccion_logica = 0;
        uint32_t posicion = 0;
        int tamanio = -1;
        int valor = -1;
        int tiempo = -1;
        int num_instruccion = -1;
        t_paquete *paquete_para_kernel_archivo = NULL;

        switch (tipo_inst(datos[0]))
        {
        case (SET):
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s - %s\n", contexto_ejecucion->pid, datos[0], datos[1], datos[2]);
            registro = datos[1];
            valor = atoi(datos[2]);
            setear_registro(registro, valor);
            //mostrar_valores(contexto_ejecucion);
            contexto_ejecucion->program_counter += 1;
            break;

        case (SUM):
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s - %s\n", contexto_ejecucion->pid, datos[0], datos[1], datos[2]);
            registro_destino = datos[1];
            registro_origen = datos[2];
            valor = sumar_registros(registro_destino, registro_origen);
            setear_registro(registro_destino, valor);
            //mostrar_valores(contexto_ejecucion);            
            contexto_ejecucion->program_counter += 1;
            break;

        case (SUB):
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s - %s\n", contexto_ejecucion->pid, datos[0], datos[1], datos[2]);
            registro_destino = datos[1];
            registro_origen = datos[2];
            valor = restar_registros(registro_destino, registro_origen);
            setear_registro(registro_destino, valor);
            //mostrar_valores(contexto_ejecucion);
            contexto_ejecucion->program_counter += 1;
            break;

        case(JNZ): 
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s - %s\n", contexto_ejecucion->pid, datos[0], datos[1], datos[2]);
            registro = datos[1];
            num_instruccion = atoi(datos[2]);
            if (registro != 0) {
                 contexto_ejecucion->program_counter = num_instruccion;
            }
            break;

        case(SLEEP):
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s\n", contexto_ejecucion->pid, datos[0], datos[1]);
            tiempo = atoi(datos[1]);
            contexto_ejecucion->program_counter += 1;
            devolver_contexto_ejecucion(socket_cliente_dispatch, contexto_ejecucion, "sleep", "", tiempo); 
            seguir_ejecutando = false;
            break;

        case (WAIT):
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s\n", contexto_ejecucion->pid, datos[0], datos[1]);
            recurso = datos[1];
            //mostrar_valores(contexto_ejecucion);
            contexto_ejecucion->program_counter += 1;
            devolver_contexto_ejecucion(socket_cliente_dispatch, contexto_ejecucion, "wait",recurso, 0);
            seguir_ejecutando = false;
            break;

        case (SIGNAL):
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s\n", contexto_ejecucion->pid, datos[0], datos[1]);
            recurso = datos[1];
            //mostrar_valores(contexto_ejecucion);
            contexto_ejecucion->program_counter += 1;
            devolver_contexto_ejecucion(socket_cliente_dispatch, contexto_ejecucion, "signal", recurso, 0);
            seguir_ejecutando = false;
            break;

        case(MOV_IN):
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s - %s\n", contexto_ejecucion->pid, datos[0], datos[1], datos[2]);
            registro = datos[1];
            //mostrar_valores(contexto_ejecucion);
            direccion_logica = atoi(datos[2]);
            contexto_ejecucion->program_counter += 1;
            break;
        
        case(MOV_OUT):
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s - %s\n", contexto_ejecucion->pid, datos[0], datos[1], datos[2]);
            registro = datos[2];
            //mostrar_valores(contexto_ejecucion);
            direccion_logica = atoi(datos[1]);
            contexto_ejecucion->program_counter += 1;
            break;

        case(F_OPEN):
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s - %s\n", contexto_ejecucion->pid, datos[0], datos[1], datos[2]);
            nombre_archivo = datos[1];
            modo_apertura = datos[2];

            //bloqueamos el proceso
            
            //mandamos el pawquete 
            paquete_para_kernel_archivo = crear_paquete(ABRIR_ARCHIVO);
            agregar_cadena_a_paquete(paquete_para_kernel_archivo, nombre_archivo);
            agregar_cadena_a_paquete(paquete_para_kernel_archivo, modo_apertura);
            enviar_paquete(paquete_para_kernel_archivo, socket_servidor_dispatch);
            eliminar_paquete(paquete_para_kernel_archivo);
            //mandarle el contexto de ejecucion (creo)
            
            contexto_ejecucion->program_counter += 1;
            devolver_contexto_ejecucion(socket_cliente_dispatch, contexto_ejecucion, "f_open", recurso, 0);
            seguir_ejecutando = false;
            break;

        case(F_CLOSE):
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s\n", contexto_ejecucion->pid, datos[0], datos[1]);
            nombre_archivo = datos[1];

            //bloqueamos el proceso
            
            //una vez bloqueado el proceso mandamos el paquete
            printf("\n\nArmamos paquete para enviar a CPU\n\n");
            paquete_para_kernel_archivo = crear_paquete(CERRAR_ARCHIVO);
            agregar_cadena_a_paquete(paquete_para_kernel_archivo, nombre_archivo);
            //mandarle el contexto de ejecucion (creo)
            enviar_paquete(paquete_para_kernel_archivo, socket_servidor_dispatch);
            eliminar_paquete(paquete_para_kernel_archivo);
            printf("\n\nEnviamos paquete a kernel para CERRAR ARCHIVO\n\n");

            mostrar_valores(contexto_ejecucion);
            contexto_ejecucion->program_counter += 1;
            break;

        case(F_TRUNCATE):
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s - %s\n", contexto_ejecucion->pid, datos[0], datos[1], datos[2]);
            nombre_archivo = datos[1];
            tamanio = atoi(datos[2]);

            //bloqueamos el proceso
            
            //una vez bloqueado el proceso mandamos el paquete
            printf("\n\nArmamos paquete para enviar a CPU\n\n");
            paquete_para_kernel_archivo = crear_paquete(TRUNCAR_ARCHIVO);
            agregar_cadena_a_paquete(paquete_para_kernel_archivo, nombre_archivo);
            agregar_entero_a_paquete(paquete_para_kernel_archivo, tamanio);
            //mandarle el contexto de ejecucion (creo)
            enviar_paquete(paquete_para_kernel_archivo, socket_servidor_dispatch);
            eliminar_paquete(paquete_para_kernel_archivo);
            printf("\n\nEnviamos paquete a kernel para TRUNCAR ARCHIVO\n\n");

            mostrar_valores(contexto_ejecucion);
            contexto_ejecucion->program_counter += 1;
            break;
        
        case(F_SEEK):
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s - %s\n", contexto_ejecucion->pid, datos[0], datos[1], datos[2]);
            nombre_archivo = datos[1];
            posicion = atoi(datos[2]);

            //bloqueamos el proceso
            
            //una vez bloqueado el proceso mandamos el paquete
            printf("\n\nArmamos paquete para enviar a CPU\n\n");
            paquete_para_kernel_archivo = crear_paquete(BUSCAR_ARCHIVO);
            agregar_cadena_a_paquete(paquete_para_kernel_archivo, nombre_archivo);
            agregar_entero_a_paquete(paquete_para_kernel_archivo, posicion);
            //mandarle el contexto de ejecucion (creo)
            enviar_paquete(paquete_para_kernel_archivo, socket_servidor_dispatch);
            eliminar_paquete(paquete_para_kernel_archivo);
            printf("\n\nEnviamos paquete a kernel para POSICIONARNOS EN ARCHIVO\n\n");

            mostrar_valores(contexto_ejecucion);
            contexto_ejecucion->program_counter += 1;
            break;

        case(F_READ):
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s - %s\n", contexto_ejecucion->pid, datos[0], datos[1], datos[2]);
            nombre_archivo = datos[1];
            direccion_logica = atoi(datos[2]);
            mostrar_valores(contexto_ejecucion);
            contexto_ejecucion->program_counter += 1;
            break;

        case(F_WRITE):
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s - %s\n", contexto_ejecucion->pid, datos[0], datos[1], datos[2]);
            nombre_archivo = datos[1];
            direccion_logica = atoi(datos[2]);
            mostrar_valores(contexto_ejecucion);
            contexto_ejecucion->program_counter += 1;
            break;

        case (INSTRUCCION_EXIT):
            log_info(cpu_logger, "PID: %d - Ejecutando: %s\n", contexto_ejecucion->pid, datos[0]);
            devolver_contexto_ejecucion(socket_cliente_dispatch, contexto_ejecucion, "exit","", 0);
            // eliminar_todas_las_entradas(contexto_ejecucion->pid);
            seguir_ejecutando = false;
            break;
        } 

        //CHECK INTERRUPT
        //printf("num_interrupcion =  %d  \n", interrupcion); 
        if(hay_interrupcion() && tipo_interrupcion == 1) //&& seguir ejecutando?
        {
            //printf("\nDetectamos interrupcion\n");
            pthread_mutex_lock(&mutex_interrupcion);
            interrupcion = 0;
            pthread_mutex_unlock(&mutex_interrupcion);
            devolver_contexto_ejecucion(socket_cliente_dispatch, contexto_ejecucion, "desalojo", "", 0);
            seguir_ejecutando = false;
        }else if (hay_interrupcion() && tipo_interrupcion == 2)
        {
            printf("\nDetectamos interrupcion\n");
            pthread_mutex_lock(&mutex_interrupcion);
            interrupcion = 0;
            pthread_mutex_unlock(&mutex_interrupcion);
            devolver_contexto_ejecucion(socket_cliente_dispatch, contexto_ejecucion, "finalizacion", "", 0);
            seguir_ejecutando = false;
        }else if (hay_interrupcion() && tipo_interrupcion == 3) 
        {
            //log_info(cpu_logger, "Oh no hermano, tenemos Page Fault: PID %d - Número de página %d", contexto_ejecucion->pid, contexto_ejecucion->pag_pf);
            printf("\nDetectamos interrupcion\n");
            pthread_mutex_lock(&mutex_interrupcion);
            interrupcion = 0;
            pthread_mutex_unlock(&mutex_interrupcion);
            devolver_contexto_ejecucion(socket_cliente_dispatch, contexto_ejecucion, "page_fault", "", 0);
            //como tengo page fault notificó al kernel y no puedo seguir ejecutando el proceso hasta manejar el page fault
            seguir_ejecutando = false;
        }


        string_array_destroy(datos);
    }

    free(contexto_ejecucion->recursos_asignados);

}
    
//================================================== PAGE FAULT ===================================================================
// verifica si se ha producido un page fault esto recién para el checkpoint 4
/*bool hay_page_fault() {
    //desallorar la lógica para el page fault
    //return (interrupcion == PAGE_FAULT); // suponiendo que PAGE_FAULT es un valor que nos va a indicar page fault.
}

// nos va a decir el n° de página por el que se generó el page fault
int obtener_numero_pagina(char *instruccion) {
    
}
*/
//================================================== Interrupt =====================================================================

// este canal se va a usar para mensajes de interrupcion
void atender_interrupt(void *socket_servidor_interrupt)
{
    int conexion = *(int *)socket_servidor_interrupt;

    while (1)
    {
        t_paquete *paquete = recibir_paquete(conexion);
        log_info(cpu_logger, "Recibi un aviso por interrupt del kernel");
        void *stream = paquete->buffer->stream;

        if (paquete->codigo_operacion == DESALOJO)
        {
            // esto despues lo usamos en el check interrupt para saber que si hay una interrupcion, devolvemos el contexto de ejecucion
            tipo_interrupcion = sacar_entero_de_paquete(&stream);

            // lo tenemos que poner en un semaforo para que nada lo modifique mientras le sumo 1, cosa que despues no se pierda que
            // efectivamente hubo un interrupcion
            pthread_mutex_lock(&mutex_interrupcion);
            interrupcion += 1;
            pthread_mutex_unlock(&mutex_interrupcion);
        }else if (paquete->codigo_operacion == FINALIZACION_PROCESO)
        {
            tipo_interrupcion = sacar_entero_de_paquete(&stream);
            pthread_mutex_lock(&mutex_interrupcion);
            interrupcion += 1;
            pthread_mutex_unlock(&mutex_interrupcion);
        }else if (paquete->codigo_operacion == PAGE_FAULT)
        {
            tipo_interrupcion = sacar_entero_de_paquete(&stream);
            pthread_mutex_lock(&mutex_interrupcion);
            interrupcion += 1;
            pthread_mutex_unlock(&mutex_interrupcion);   
        }else
        {
            printf("No recibi una interrupcion\n");
        }
        eliminar_paquete(paquete);
    }
}

static bool hay_interrupcion()
{
    bool seguir_ejecutando = false;

    pthread_mutex_lock(&mutex_interrupcion);
    if(interrupcion >=1)
    {
        seguir_ejecutando = true;
    }
    pthread_mutex_unlock(&mutex_interrupcion);

    return seguir_ejecutando;
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

    int absoluto = abs(resta);

    return absoluto;
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
    {
        numero = SET;
    } else if (string_equals_ignore_case(instruccion, "SUM"))
    {
        numero = SUM;
        
    } else if (string_equals_ignore_case(instruccion, "SUB"))
    {
        numero = SUB;
        
    } else if (string_equals_ignore_case(instruccion, "JNZ"))
    {
        numero = JNZ;

    } else if (string_equals_ignore_case(instruccion, "SLEEP"))
    {
        numero = SLEEP;
    } else if (string_equals_ignore_case(instruccion, "WAIT"))
    {
        numero = WAIT;
    } else if (string_equals_ignore_case(instruccion, "SIGNAL"))
    {
        numero = SIGNAL;
    } else if (string_equals_ignore_case(instruccion, "MOV_IN"))
    {
        numero = MOV_IN;

    } else if (string_equals_ignore_case(instruccion, "MOV_OUT"))
    {
        numero = MOV_OUT;

    } else if (string_equals_ignore_case(instruccion, "F_OPEN"))
    {
        numero = F_OPEN;

    } else if (string_equals_ignore_case(instruccion, "F_CLOSE"))
    {
        numero = F_CLOSE;

    } else if (string_equals_ignore_case(instruccion, "F_SEEK"))
    {
        numero = F_SEEK;

    } else if (string_equals_ignore_case(instruccion, "F_READ"))
    {
        numero = F_READ;

    } else if (string_equals_ignore_case(instruccion, "F_WRITE"))
    {
        numero = F_WRITE;

    } else if (string_equals_ignore_case(instruccion, "F_TRUNCATE"))
    {
        numero = F_TRUNCATE;

    } else if (string_equals_ignore_case(instruccion, "EXIT"))
    {
        numero = INSTRUCCION_EXIT;
    } else
    {
        log_info(cpu_logger, "no encontre la instruccion %d\n", numero);
        abort();
    }
    return numero;
}

//================================================== CONTEXTO =====================================================================

/*vamos a devolver el contexto al kernel en las instrucciones exit, sleep. Lo modifique para que podamos
pedir recursos por aca tambien, en vez de hacer una funcion aparte. Si no pedimos un recurso entonces ponemos
"" en el ultimo parametro y fuee. */
static void devolver_contexto_ejecucion(int socket_cliente, t_contexto_ejecucion *contexto_ejecucion, char *motivo, char *recurso, int tiempo)
{
    // aca nosotros agregamos las modificaciones de los registros
    (contexto_ejecucion->registros_cpu.AX) = AX;
    (contexto_ejecucion->registros_cpu.BX) = BX;
    (contexto_ejecucion->registros_cpu.CX) = CX;
    (contexto_ejecucion->registros_cpu.DX) = DX;
    enviar_contexto(socket_cliente, contexto_ejecucion, motivo, recurso, tiempo);
    log_info(cpu_logger, "Devolvi el contexto ejecucion al kernel por motivo de: %s \n", motivo);

}

static void enviar_contexto(int socket_cliente, t_contexto_ejecucion *contexto_ejecucion, char *motivo,char *recurso, int tiempo)
{
    t_paquete *paquete = crear_paquete(PCB);

    // le mandamos esto porque creo que es lo unico que se cambia pero vemos
    agregar_entero_a_paquete(paquete, contexto_ejecucion->program_counter);
    agregar_entero_sin_signo_a_paquete(paquete, contexto_ejecucion->registros_cpu.AX);
    agregar_entero_sin_signo_a_paquete(paquete, contexto_ejecucion->registros_cpu.BX);
    agregar_entero_sin_signo_a_paquete(paquete, contexto_ejecucion->registros_cpu.CX);
    agregar_entero_sin_signo_a_paquete(paquete, contexto_ejecucion->registros_cpu.DX);
    agregar_cadena_a_paquete(paquete, motivo);
    agregar_cadena_a_paquete(paquete, recurso);
    agregar_entero_a_paquete(paquete, tiempo);

    // agregar_entero_a_paquete(paquete, contexto_ejecucion->hay_que_bloquear);
    enviar_paquete(paquete, socket_cliente);
    eliminar_paquete(paquete);
}

static void mostrar_valores (t_contexto_ejecucion* contexto)
{
    // estos son los registros de la cpu que ya inicializamos arriba y almacenan valores enteros no signados de 4 bytes
    log_info(cpu_logger, "AX = %d BX = %d CX = %d DX = %d", AX, BX, CX, DX);

    for (int i = 0; i < tamanio_recursos; ++i) {
        log_info(cpu_logger, "Recursos Asignados: %s - Cantidad: %d",contexto->recursos_asignados[i].nombre_recurso, contexto->recursos_asignados[i].instancias_recurso);
    }
}
