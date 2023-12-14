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
static void enviar_handshake();
static void recibir_handshake();
static int sumar_registros(char *registro_destino, char *registro_origen);
static int restar_registros(char *registro_destino, char *registro_origen);
static int tipo_inst(char *instruccion);
static void devolver_contexto_ejecucion(t_contexto_ejecucion *contexto_ejecucion, char *motivo, char* recurso, int tiempo, int numero_pagina, char* nombre_archivo, char* modo_apertura, int tamanio_archivo);
static void enviar_contexto(t_contexto_ejecucion *contexto_ejecucion, char *motivo,char *recurso, int tiempo, int numero_pagina, char* nombre_archivo, char* modo_apertura, int tamanio_archivo);
static void pedir_instruccion(int posicion, int pid);
static void recibir_instruccion();
static bool hay_interrupcion();


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

}

//================================================== Handshake =====================================================================
void realizar_handshake()
{
    enviar_handshake();
    recibir_handshake();
}

static void enviar_handshake()
{
    t_paquete *paquete = crear_paquete(HANDSHAKE);
    agregar_entero_a_paquete(paquete, 1);
    enviar_paquete(paquete, socket_cliente_memoria);
    eliminar_paquete(paquete);
}

static void recibir_handshake()
{
    t_paquete* paquete = recibir_paquete(socket_cliente_memoria);
    void* stream = paquete->buffer->stream;
    if (paquete->codigo_operacion == HANDSHAKE)
    {
        tam_pagina = sacar_entero_de_paquete(&stream);
    }
    else
    {
        log_error(cpu_logger,"No me enviaste el tam_pagina :( \n");
        abort();
    }
    eliminar_paquete(paquete);

}

//================================================== Instrucciones =====================================================================

static void recibir_instruccion()
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

static void pedir_instruccion(int posicion, int pid)
{
    t_paquete *paquete = crear_paquete(MANDAR_INSTRUCCIONES);
    agregar_entero_a_paquete(paquete,posicion);
    agregar_entero_a_paquete(paquete,pid);
    enviar_paquete(paquete, socket_cliente_memoria);
    eliminar_paquete(paquete);
}

//================================================== Dispatch =====================================================================

void atender_dispatch()
{
    while(1) 
    {
        t_paquete *paquete = recibir_paquete(socket_cliente_dispatch);
        void *stream = paquete->buffer->stream;
        
        t_contexto_ejecucion* contexto_ejecucion = malloc(sizeof(t_contexto_ejecucion));
        contexto_ejecucion->recursos_asignados = malloc(3 *sizeof(t_recursos_asignados));

        if (paquete->codigo_operacion == PCB)
        {
            contexto_ejecucion->pid = sacar_entero_de_paquete(&stream);
            contexto_ejecucion->program_counter = sacar_entero_de_paquete(&stream);
            //printf("Kernel me dice manda  PID:%d y PC: %d\n", contexto_ejecucion->pid, contexto_ejecucion->program_counter);
            contexto_ejecucion->puntero = sacar_entero_sin_signo_de_paquete(&stream);
            contexto_ejecucion->registros_cpu.AX = sacar_entero_sin_signo_de_paquete(&stream);
            contexto_ejecucion->registros_cpu.BX = sacar_entero_sin_signo_de_paquete(&stream);
            contexto_ejecucion->registros_cpu.CX = sacar_entero_sin_signo_de_paquete(&stream);
            contexto_ejecucion->registros_cpu.DX = sacar_entero_sin_signo_de_paquete(&stream);
            contexto_ejecucion->direccion_fisica_proceso = sacar_entero_sin_signo_de_paquete(&stream);

            // Iterar sobre cada recurso y recibir la información del paquete
            for (int i = 0; i < tamanio_recursos; ++i) {

                char* nombre = sacar_cadena_de_paquete(&stream);
                strcpy(contexto_ejecucion->recursos_asignados[i].nombre_recurso,nombre); 
                contexto_ejecucion->recursos_asignados[i].instancias_recurso = sacar_entero_de_paquete(&stream);
                free(nombre);
            }

            // iniciamos el procedimiento para procesar cualquier instruccion
            ciclo_de_instruccion(contexto_ejecucion);
        }
        if(paquete->codigo_operacion != PCB)
        {
        perror("No se recibio correctamente el PCB");  
        abort();
        }

        free(contexto_ejecucion);
        eliminar_paquete(paquete);
    }
}

//================================================== Ciclo de Instruccion =====================================================================

void ciclo_de_instruccion(t_contexto_ejecucion *contexto_ejecucion)
{
    bool seguir_ejecutando = true;
    bool ejecuto_instruccion = true;
    uint32_t direccion_logica_aux;
    hay_page_fault = false;

    while (seguir_ejecutando) // definir con cuentas voy, definir con program counter
    {
        uint32_t direccion_fisica;

        //=============================================== FETCH =================================================================
        log_info(cpu_logger, "PID: %d - FETCH - Program Counter: %d", contexto_ejecucion->pid, contexto_ejecucion->program_counter);

        log_info(cpu_logger, "AX = %d BX = %d CX = %d DX = %d\n", AX, BX, CX, DX);

        //le mando el program pointer a la memoria para que me pase la instruccion a la que apunta
        pedir_instruccion(contexto_ejecucion->program_counter, contexto_ejecucion->pid);

        //una vez que la recibo de memoria, la guardo en la var global de arriba
        recibir_instruccion();
        
        char **datos = string_split(instruccion, " ");
     
        //=============================================== DECODE =================================================================
        if(string_starts_with(instruccion, "MOV_OUT"))
        {

        log_info(cpu_logger,"%s requiere traduccion", instruccion);
        uint32_t direccion_logica = atoi(datos[1]);
        direccion_logica_aux = direccion_logica;
        direccion_fisica = traducir_de_logica_a_fisica(direccion_logica, contexto_ejecucion);
        log_info(cpu_logger, "Se tradujo %d", direccion_fisica);  

        }else if(string_starts_with(instruccion, "MOV_IN") || string_starts_with(instruccion, "F_READ") || string_starts_with(instruccion, "F_WRITE")){
            
        log_info(cpu_logger,"%s requiere traduccion", instruccion);
        uint32_t direccion_logica = atoi(datos[2]);
        direccion_logica_aux = direccion_logica;
        direccion_fisica = traducir_de_logica_a_fisica(direccion_logica, contexto_ejecucion);
        log_info(cpu_logger, "Se tradujo la direccion %d", direccion_fisica);      

        }   
        
        if (hay_page_fault) 
        {
            int numero_pagina = floor(direccion_logica_aux / tam_pagina);
            log_info(cpu_logger, "Page Fault: PID %d - Pagina %d", contexto_ejecucion->pid, numero_pagina);
            devolver_contexto_ejecucion(contexto_ejecucion, "page_fault", "", 0, numero_pagina, "", "", -1);
            seguir_ejecutando = false;
            ejecuto_instruccion = false;
            hay_page_fault = false;
        }
        
        
        //=============================================== EXECUTE =================================================================

        // toda esta parte la usamos para trabajar con registros (sumar,restar,poner)
        free(instruccion);
        char *registro_SET = NULL;
        char* registro_MOV_IN = NULL;
        char* registro_JNZ = NULL;
        char* registro_MOV_OUT = NULL;
        char *registro_destino_SUM = NULL;
        char *registro_origen_SUM = NULL;
        char *registro_destino_SUB = NULL;
        char *registro_origen_SUB = NULL;
        char *recurso_wait = NULL;
        char *recurso_signal = NULL;
        char* modo_apertura_f_open = NULL;
        char* nombre_archivo_f_open = NULL;
        char* nombre_archivo_f_write = NULL;
        char* nombre_archivo_f_seek = NULL;
        char* nombre_archivo_f_read = NULL;
        char* nombre_archivo_f_close = NULL;
        char* nombre_archivo_f_truncate = NULL;
        uint32_t posicion = 0;
        int tamanio = -1;
        int valor_SET = -1;
        int valor_SUM = -1;
        int valor_SUB = -1;
        int tiempo = -1;
        int num_instruccion = -1;

        switch (tipo_inst(datos[0]))
        {
        case (SET):
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s - %s\n", contexto_ejecucion->pid, datos[0], datos[1], datos[2]);
            registro_SET = datos[1];
            valor_SET = atoi(datos[2]);
            setear_registro(registro_SET, valor_SET);
            contexto_ejecucion->program_counter += 1;
            break;

        case (SUM):
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s - %s\n", contexto_ejecucion->pid, datos[0], datos[1], datos[2]);
            registro_destino_SUM = datos[1];
            registro_origen_SUM = datos[2];
            valor_SUM = sumar_registros(registro_destino_SUM, registro_origen_SUM);
            setear_registro(registro_destino_SUM, valor_SUM);
            contexto_ejecucion->program_counter += 1;
            break;

        case (SUB):
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s - %s\n", contexto_ejecucion->pid, datos[0], datos[1], datos[2]);
            registro_destino_SUB = datos[1];
            registro_origen_SUB = datos[2];
            valor_SUB = restar_registros(registro_destino_SUB, registro_origen_SUB);
            setear_registro(registro_destino_SUB, valor_SUB);
            contexto_ejecucion->program_counter += 1;
            break;

        case(JNZ): 
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s - %s\n", contexto_ejecucion->pid, datos[0], datos[1], datos[2]);
            registro_JNZ = datos[1];
            num_instruccion = atoi(datos[2]);
            int valor_registro_JNZ = buscar_registro(registro_JNZ);
            if (valor_registro_JNZ != 0 && valor_registro_JNZ > 0 ) {
                 contexto_ejecucion->program_counter = num_instruccion;
            } else {
                contexto_ejecucion->program_counter += 1;
            }
            //printf("JNZ me coloca  PID:%d y PC: %d\n", contexto_ejecucion->pid, contexto_ejecucion->program_counter);            
            break;

        case(SLEEP):
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s\n", contexto_ejecucion->pid, datos[0], datos[1]);
            tiempo = atoi(datos[1]);
            contexto_ejecucion->program_counter += 1;
            devolver_contexto_ejecucion(contexto_ejecucion, "sleep", "basura", tiempo, -1, "basura", "basura",-1); 
            seguir_ejecutando = false;
            break;

        case (WAIT):
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s\n", contexto_ejecucion->pid, datos[0], datos[1]);
            recurso_wait = datos[1];
            contexto_ejecucion->program_counter += 1;
            devolver_contexto_ejecucion(contexto_ejecucion, "wait",recurso_wait, 0, -1, "basura", "basura", -1);
            seguir_ejecutando = false;
            break;

        case (SIGNAL):
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s\n", contexto_ejecucion->pid, datos[0], datos[1]);
            recurso_signal = datos[1];
            contexto_ejecucion->program_counter += 1;
            devolver_contexto_ejecucion(contexto_ejecucion, "signal", recurso_signal, 0, -1, "basura", "basura",-1);
            seguir_ejecutando = false;
            break;

        case(MOV_IN):
            if(ejecuto_instruccion){
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s - %s\n", contexto_ejecucion->pid, datos[0], datos[1], datos[2]);
            registro_MOV_IN = datos[1];;
            mov_in(registro_MOV_IN, direccion_fisica, contexto_ejecucion ,direccion_logica_aux);
            contexto_ejecucion->program_counter += 1;
            }
            break;
        
        case(MOV_OUT):
            if(ejecuto_instruccion){
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s - %s\n", contexto_ejecucion->pid, datos[0], datos[1], datos[2]);
            registro_MOV_OUT = datos[2];
            mov_out(direccion_fisica, registro_MOV_OUT, contexto_ejecucion, direccion_logica_aux); 
            contexto_ejecucion->program_counter += 1;
            }
            break;

        case(F_OPEN):
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s - %s\n", contexto_ejecucion->pid, datos[0], datos[1], datos[2]);
            nombre_archivo_f_open = datos[1];
            modo_apertura_f_open = datos[2];
            contexto_ejecucion->program_counter += 1;
            devolver_contexto_ejecucion(contexto_ejecucion, "f_open", "basura", 0, -1, nombre_archivo_f_open, modo_apertura_f_open,-1);
            seguir_ejecutando = false;
            break;

        case(F_CLOSE):
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s\n", contexto_ejecucion->pid, datos[0], datos[1]);
            nombre_archivo_f_close = datos[1];
            contexto_ejecucion->program_counter += 1;
            devolver_contexto_ejecucion(contexto_ejecucion, "f_close", "basura", 0, -1, nombre_archivo_f_close, "basura",-1);
            seguir_ejecutando = false;
            break;

        case(F_TRUNCATE):
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s - %s\n", contexto_ejecucion->pid, datos[0], datos[1], datos[2]);
            nombre_archivo_f_truncate = datos[1];
            tamanio = atoi(datos[2]); 
            contexto_ejecucion->program_counter += 1;
            devolver_contexto_ejecucion(contexto_ejecucion, "f_truncate", "basura", 0, -1, nombre_archivo_f_truncate, "basura", tamanio);
            seguir_ejecutando = false;
            break;
        
        case(F_SEEK):
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s - %s\n", contexto_ejecucion->pid, datos[0], datos[1], datos[2]);
            nombre_archivo_f_seek = datos[1];
            posicion = atoi(datos[2]);
            contexto_ejecucion->puntero = posicion;
            contexto_ejecucion->program_counter += 1;
            devolver_contexto_ejecucion(contexto_ejecucion, "f_seek", "basura", 0, -1, nombre_archivo_f_seek, "basura", -1);
            seguir_ejecutando = false;
            break;

        case(F_READ):
            if(ejecuto_instruccion){
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s - %s\n", contexto_ejecucion->pid, datos[0], datos[1], datos[2]);
            nombre_archivo_f_read = datos[1];
            contexto_ejecucion->direccion_fisica_proceso = direccion_fisica;
            contexto_ejecucion->program_counter += 1;
            devolver_contexto_ejecucion(contexto_ejecucion, "f_read", "basura", 0, -1, nombre_archivo_f_read, "basura", -1);
            seguir_ejecutando = false;
            }
            break;

        case(F_WRITE):
            if(ejecuto_instruccion){
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s - %s\n", contexto_ejecucion->pid, datos[0], datos[1], datos[2]);
            nombre_archivo_f_write = datos[1];
            contexto_ejecucion->direccion_fisica_proceso = direccion_fisica;
            contexto_ejecucion->program_counter += 1;
            devolver_contexto_ejecucion(contexto_ejecucion, "f_write", "basura", 0, -1, nombre_archivo_f_write, "basura", -1);
            seguir_ejecutando = false;
            }
            break;

        case (INSTRUCCION_EXIT):
            log_info(cpu_logger, "PID: %d - Ejecutando: %s\n", contexto_ejecucion->pid, datos[0]);
            devolver_contexto_ejecucion(contexto_ejecucion, "exit","basura", 0, -1, "basura", "basura", -1);
            seguir_ejecutando = false;
            break;
        }

        //CHECK INTERRUPT
        if(hay_interrupcion() && tipo_interrupcion == 1) //POSIBLE CONDICION
        {
            pthread_mutex_lock(&mutex_interrupcion);
            interrupcion = 0;
            pthread_mutex_unlock(&mutex_interrupcion);
            devolver_contexto_ejecucion(contexto_ejecucion, "desalojo", "basura", 0, -1, "basura", "basura",-1);
            //printf("Desalojo me manda  PID:%d y PC: %d\n", contexto_ejecucion->pid, contexto_ejecucion->program_counter);
            seguir_ejecutando = false;
        } else if(hay_interrupcion() && tipo_interrupcion == 2) {
            pthread_mutex_lock(&mutex_interrupcion);
            interrupcion = 0;
            pthread_mutex_unlock(&mutex_interrupcion);
            devolver_contexto_ejecucion(contexto_ejecucion, "finalizacion", "basura", 0, -1, "basura", "basura",-1);
            seguir_ejecutando = false;
        } 
        string_array_destroy(datos);
    }

    free(contexto_ejecucion->recursos_asignados);

}
    
//================================================== Interrupt =====================================================================

void atender_interrupt(void *socket_servidor_interrupt) 
{
    int conexion = *(int *)socket_servidor_interrupt;

    while (1)
    {
        t_paquete *paquete = recibir_paquete(conexion);
        void *stream = paquete->buffer->stream;

        if (paquete->codigo_operacion == DESALOJO)
        {
            tipo_interrupcion = sacar_entero_de_paquete(&stream); 

            pthread_mutex_lock(&mutex_interrupcion);
            interrupcion += 1;
            pthread_mutex_unlock(&mutex_interrupcion);
        }else if (paquete->codigo_operacion == FINALIZAR_PROCESO)
        {
            tipo_interrupcion = sacar_entero_de_paquete(&stream);
            pthread_mutex_lock(&mutex_interrupcion);
            interrupcion += 1;
            pthread_mutex_unlock(&mutex_interrupcion);
        } else {
            printf("No recibi una interrupcion\n");
            abort();
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
void setear_registro(char *registros, int valor)
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

int buscar_registro(char *registros)
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

static void devolver_contexto_ejecucion(t_contexto_ejecucion *contexto_ejecucion, char *motivo, char *recurso, int tiempo, int numero_pagina, char* nombre_archivo, char* modo_apertura, int tamanio_archivo)
{
    // aca nosotros agregamos las modificaciones de los registros
    (contexto_ejecucion->registros_cpu.AX) = AX;
    (contexto_ejecucion->registros_cpu.BX) = BX;
    (contexto_ejecucion->registros_cpu.CX) = CX;
    (contexto_ejecucion->registros_cpu.DX) = DX;
    enviar_contexto(contexto_ejecucion, motivo, recurso, tiempo, numero_pagina, nombre_archivo, modo_apertura, tamanio_archivo);
}

static void enviar_contexto(t_contexto_ejecucion *contexto_ejecucion, char *motivo,char *recurso, int tiempo, int numero_pagina, char* nombre_archivo, char* modo_apertura, int tamanio_archivo )
{
    t_paquete *paquete = crear_paquete(PCB);

    agregar_entero_a_paquete(paquete, contexto_ejecucion->program_counter);
    //printf("CPU devuelve PID:%d y PC: %d\n", contexto_ejecucion->pid, contexto_ejecucion->program_counter);
    agregar_entero_sin_signo_a_paquete(paquete, contexto_ejecucion->registros_cpu.AX);
    agregar_entero_sin_signo_a_paquete(paquete, contexto_ejecucion->registros_cpu.BX);
    agregar_entero_sin_signo_a_paquete(paquete, contexto_ejecucion->registros_cpu.CX);
    agregar_entero_sin_signo_a_paquete(paquete, contexto_ejecucion->registros_cpu.DX);
    agregar_cadena_a_paquete(paquete, motivo);
    agregar_cadena_a_paquete(paquete, recurso);
    agregar_entero_a_paquete(paquete, tiempo);
    agregar_entero_a_paquete(paquete, numero_pagina);
    agregar_cadena_a_paquete(paquete, nombre_archivo);
    agregar_cadena_a_paquete(paquete, modo_apertura);
    agregar_entero_sin_signo_a_paquete(paquete, contexto_ejecucion->puntero);
    agregar_entero_sin_signo_a_paquete(paquete, contexto_ejecucion->direccion_fisica_proceso);
    agregar_entero_a_paquete(paquete, tamanio_archivo);

    enviar_paquete(paquete, socket_cliente_dispatch);
    eliminar_paquete(paquete);
}
