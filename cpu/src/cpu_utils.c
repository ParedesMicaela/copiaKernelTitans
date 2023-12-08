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
static int sumar_registros(char *registro_destino, char *registro_origen);
static int restar_registros(char *registro_destino, char *registro_origen);
static int tipo_inst(char *instruccion);
static void devolver_contexto_ejecucion(int socket_cliente, t_contexto_ejecucion *contexto_ejecucion, char *motivo, char* recurso, int tiempo, int numero_pagina, char* nombre_archivo, char* modo_apertura, int posicion, uint32_t direccion_fisica_proceso, int tamanio_archivo);
static void enviar_contexto(int socket_cliente, t_contexto_ejecucion *contexto_ejecucion, char *motivo,char *recurso, int tiempo, int numero_pagina, char* nombre_archivo, char* modo_apertura, int posicion, uint32_t direccion_fisica_proceso, int tamanio_archivo);
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
    bool ejecuto_instruccion = true;
    uint32_t direccion_logica_aux;
    int numero_pagina_aux;
    hay_page_fault = false;

    while (seguir_ejecutando) // definir con cuentas voy, definir con program counter
    {
        uint32_t direccion_fisica;

        //=============================================== FETCH =================================================================
        log_info(cpu_logger, "PID: %d - FETCH - Program Counter: %d", contexto_ejecucion->pid, contexto_ejecucion->program_counter);

        log_info(cpu_logger, "AX = %d BX = %d CX = %d DX = %d\n", AX, BX, CX, DX);

        //le mando el program pointer a la memoria para que me pase la instruccion a la que apunta
        pedir_instruccion(socket_cliente_memoria, contexto_ejecucion->program_counter, contexto_ejecucion->pid);

        //una vez que la recibo de memoria, la guardo en la var global de arriba
        recibir_instruccion(socket_cliente_memoria);
        
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
            numero_pagina_aux = numero_pagina;
            log_info(cpu_logger, "Page Fault: PID %d - Pagina %d", contexto_ejecucion->pid, numero_pagina);
            devolver_contexto_ejecucion(socket_cliente_dispatch, contexto_ejecucion, "page_fault", "", 0, numero_pagina, "", "", -1 , 0, -1);
            seguir_ejecutando = false;
            ejecuto_instruccion = false;
            hay_page_fault = false;
        }
        
        //=============================================== EXECUTE =================================================================

        // toda esta parte la usamos para trabajar con registros (sumar,restar,poner)
        free(instruccion);
        char *registro = NULL;
        char *registro_destino = NULL;
        char *registro_origen = NULL;
        char *recurso = NULL;
        char* modo_apertura = NULL;
        char* nombre_archivo = NULL;
        uint32_t posicion = 0;
        int tamanio = -1;
        int valor = -1;
        int tiempo = -1;
        int num_instruccion = -1;

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
            devolver_contexto_ejecucion(socket_cliente_dispatch, contexto_ejecucion, "sleep", "", tiempo, -1, "", "", -1 , 0, -1); 
            seguir_ejecutando = false;
            break;

        case (WAIT):
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s\n", contexto_ejecucion->pid, datos[0], datos[1]);
            recurso = datos[1];
            //mostrar_valores(contexto_ejecucion);
            contexto_ejecucion->program_counter += 1;
            devolver_contexto_ejecucion(socket_cliente_dispatch, contexto_ejecucion, "wait",recurso, 0, -1, "", "", -1 , 0, -1);
            seguir_ejecutando = false;
            break;

        case (SIGNAL):
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s\n", contexto_ejecucion->pid, datos[0], datos[1]);
            recurso = datos[1];
            //mostrar_valores(contexto_ejecucion);
            contexto_ejecucion->program_counter += 1;
            devolver_contexto_ejecucion(socket_cliente_dispatch, contexto_ejecucion, "signal", recurso, 0, -1, "", "", -1 , 0, -1);
            seguir_ejecutando = false;
            break;

        case(MOV_IN):
            if(ejecuto_instruccion){
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s - %s\n", contexto_ejecucion->pid, datos[0], datos[1], datos[2]);
            registro = datos[1];;
            mov_in(registro, direccion_fisica, contexto_ejecucion);
            //mostrar_valores(contexto_ejecucion)
            contexto_ejecucion->program_counter += 1;
            }
            break;
        
        case(MOV_OUT):
            if(ejecuto_instruccion){
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s - %s\n", contexto_ejecucion->pid, datos[0], datos[1], datos[2]);
            registro = datos[2];
            mov_out(direccion_fisica, registro, contexto_ejecucion); 
            //mostrar_valores(contexto_ejecucion);
            contexto_ejecucion->program_counter += 1;
            }
            break;

        case(F_OPEN):
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s - %s\n", contexto_ejecucion->pid, datos[0], datos[1], datos[2]);
            nombre_archivo = datos[1];
            modo_apertura = datos[2];
            contexto_ejecucion->program_counter += 1;
            devolver_contexto_ejecucion(socket_cliente_dispatch, contexto_ejecucion, "f_open", "", 0, -1, nombre_archivo, modo_apertura, -1, 0, -1);
            seguir_ejecutando = false;
            break;

        case(F_CLOSE):
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s\n", contexto_ejecucion->pid, datos[0], datos[1]);
            nombre_archivo = datos[1];
            contexto_ejecucion->program_counter += 1;
            devolver_contexto_ejecucion(socket_cliente_dispatch, contexto_ejecucion, "f_close", "", 0, -1, nombre_archivo, "", -1, 0, -1);
            seguir_ejecutando = false;
            break;

        case(F_TRUNCATE):
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s - %s\n", contexto_ejecucion->pid, datos[0], datos[1], datos[2]);
            nombre_archivo = datos[1];
            tamanio = atoi(datos[2]); // el tamanio va a ser el nuevo tamanio del archivo
            contexto_ejecucion->program_counter += 1;
            devolver_contexto_ejecucion(socket_cliente_dispatch, contexto_ejecucion, "f_truncate", "", 0, -1, nombre_archivo, "", -1, 0, tamanio);
            seguir_ejecutando = false;
            break;
        
        case(F_SEEK):
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s - %s\n", contexto_ejecucion->pid, datos[0], datos[1], datos[2]);
            nombre_archivo = datos[1];
            posicion = atoi(datos[2]);
            contexto_ejecucion->program_counter += 1;
            devolver_contexto_ejecucion(socket_cliente_dispatch, contexto_ejecucion, "f_seek", "", 0, -1, nombre_archivo, "", posicion, 0, -1);
            seguir_ejecutando = false;
            break;

        case(F_READ):
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s - %s\n", contexto_ejecucion->pid, datos[0], datos[1], datos[2]);
            nombre_archivo = datos[1];
            contexto_ejecucion->program_counter += 1;
            devolver_contexto_ejecucion(socket_cliente_dispatch, contexto_ejecucion, "f_read", "", 0, -1, nombre_archivo, "", -1, direccion_fisica, -1);
            seguir_ejecutando = false;
            break;

        case(F_WRITE):
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s - %s\n", contexto_ejecucion->pid, datos[0], datos[1], datos[2]);
            nombre_archivo = datos[1];
            contexto_ejecucion->program_counter += 1;
            devolver_contexto_ejecucion(socket_cliente_dispatch, contexto_ejecucion, "f_write", "", 0, -1, nombre_archivo, "", -1, direccion_fisica, -1);
            seguir_ejecutando = false;
            break;

        case (INSTRUCCION_EXIT):
            log_info(cpu_logger, "PID: %d - Ejecutando: %s\n", contexto_ejecucion->pid, datos[0]);
            devolver_contexto_ejecucion(socket_cliente_dispatch, contexto_ejecucion, "exit","", 0, -1, "", "", -1 , 0, -1);
            // eliminar_todas_las_entradas(contexto_ejecucion->pid);
            seguir_ejecutando = false;
            break;
        } 

        //CHECK INTERRUPT
        if(hay_interrupcion() && tipo_interrupcion == 1) 
        {
            //printf("\nDetectamos interrupcion\n");
            pthread_mutex_lock(&mutex_interrupcion);
            interrupcion = 0;
            pthread_mutex_unlock(&mutex_interrupcion);
            devolver_contexto_ejecucion(socket_cliente_dispatch, contexto_ejecucion, "desalojo", "", 0, -1, "", "", -1 , 0, -1);
            seguir_ejecutando = false;
        }else if (hay_interrupcion() && tipo_interrupcion == 2)
        {
            printf("\nDetectamos interrupcion\n");
            pthread_mutex_lock(&mutex_interrupcion);
            interrupcion = 0;
            pthread_mutex_unlock(&mutex_interrupcion);
            devolver_contexto_ejecucion(socket_cliente_dispatch, contexto_ejecucion, "finalizacion", "", 0, -1, "", "", -1 , 0, -1);
            seguir_ejecutando = false;
        }else if (hay_interrupcion() && tipo_interrupcion == 3) 
        {
            //log_info(cpu_logger, "Oh no hermano, tenemos Page Fault: PID %d - Número de página %d", contexto_ejecucion->pid, contexto_ejecucion->pag_pf);
            printf("\nDetectamos interrupcion\n");
            pthread_mutex_lock(&mutex_interrupcion);
            interrupcion = 0;
            pthread_mutex_unlock(&mutex_interrupcion);
            devolver_contexto_ejecucion(socket_cliente_dispatch, contexto_ejecucion, "page_fault", "", 0, numero_pagina_aux, "", "", -1 , 0, -1);
            //como tengo page fault notificó al kernel y no puedo seguir ejecutando el proceso hasta manejar el page fault
            seguir_ejecutando = false;
        }


        string_array_destroy(datos);
    }

    free(contexto_ejecucion->recursos_asignados);

}
    
//================================================== Interrupt =====================================================================

// este canal se va a usar para mensajes de interrupcion
void atender_interrupt(void *socket_servidor_interrupt)
{
    int conexion = *(int *)socket_servidor_interrupt;

    while (1)
    {
        t_paquete *paquete = recibir_paquete(conexion);
        //log_info(cpu_logger, "Recibi un aviso por interrupt del kernel");
        printf("Recibi un aviso por interrupt del kernel");
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

    int absoluto = abs(resta);

    return absoluto;
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

/*vamos a devolver el contexto al kernel en las instrucciones exit, sleep. Lo modifique para que podamos
pedir recursos por aca tambien, en vez de hacer una funcion aparte. Si no pedimos un recurso entonces ponemos
"" en el ultimo parametro y fuee. */
static void devolver_contexto_ejecucion(int socket_cliente, t_contexto_ejecucion *contexto_ejecucion, char *motivo, char *recurso, int tiempo, int numero_pagina, char* nombre_archivo, char* modo_apertura, int posicion, uint32_t direccion_fisica_proceso, int tamanio_archivo)
{
    // aca nosotros agregamos las modificaciones de los registros
    (contexto_ejecucion->registros_cpu.AX) = AX;
    (contexto_ejecucion->registros_cpu.BX) = BX;
    (contexto_ejecucion->registros_cpu.CX) = CX;
    (contexto_ejecucion->registros_cpu.DX) = DX;
    enviar_contexto(socket_cliente, contexto_ejecucion, motivo, recurso, tiempo, numero_pagina, nombre_archivo, modo_apertura, posicion, direccion_fisica_proceso, tamanio_archivo);
    log_info(cpu_logger, "Devolvi el contexto ejecucion al kernel por motivo de: %s \n", motivo);

}

static void enviar_contexto(int socket_cliente, t_contexto_ejecucion *contexto_ejecucion, char *motivo,char *recurso, int tiempo, int numero_pagina, char* nombre_archivo, char* modo_apertura, int posicion, uint32_t direccion_fisica_proceso, int tamanio_archivo )
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
    agregar_entero_a_paquete(paquete, numero_pagina);
    agregar_cadena_a_paquete(paquete, nombre_archivo);
    agregar_cadena_a_paquete(paquete, modo_apertura);
    agregar_entero_a_paquete(paquete, posicion);
    agregar_entero_sin_signo_a_paquete(paquete, direccion_fisica_proceso);
    agregar_entero_a_paquete(paquete, tamanio_archivo);

    // agregar_entero_a_paquete(paquete, contexto_ejecucion->hay_que_bloquear);
    enviar_paquete(paquete, socket_cliente);
    eliminar_paquete(paquete);
}

//================================================== FUNCIONES_AUXILIARES =====================================================================

static void mostrar_valores (t_contexto_ejecucion* contexto)
{
    // estos son los registros de la cpu que ya inicializamos arriba y almacenan valores enteros no signados de 4 bytes
    log_info(cpu_logger, "AX = %d BX = %d CX = %d DX = %d", AX, BX, CX, DX);

    for (int i = 0; i < tamanio_recursos; ++i) {
        log_info(cpu_logger, "Recursos Asignados: %s - Cantidad: %d",contexto->recursos_asignados[i].nombre_recurso, contexto->recursos_asignados[i].instancias_recurso);
    }
}