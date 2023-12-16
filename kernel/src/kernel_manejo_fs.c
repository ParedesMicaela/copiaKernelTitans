#include "kernel.h"

sem_t instruccion_tipo_archivo;

t_list *tabla_global_archivos_abiertos;
bool existe_en_tabla = true;
 
int cola_locks_lectura; //tiene a todos los archivos que se estan leyendo
op_code devuelto_por=-1;

static int tipo_motivo(char *motivo);
//=============================================================================================================

void iniciar_tabla_archivos_abiertos()
{
    tabla_global_archivos_abiertos = list_create();
}

void atender_peticiones_al_fs(t_pcb* proceso)
{
    liberar_memoria(&proceso->recurso_pedido);

    int tamanio = 0 ;
    uint32_t direccion_fisica = 0;
    existe_en_tabla = false;

    char* motivo_bloqueo = string_duplicate(proceso->motivo_bloqueo);

    liberar_memoria(&proceso->motivo_bloqueo);

    switch(tipo_motivo(motivo_bloqueo)){  

    case ABRIR_ARCHIVO:
        free(motivo_bloqueo);
        log_info(kernel_logger, "PID: %d - Abrir Archivo: %s Modo Apertura: %s", proceso->pid, proceso->nombre_archivo, proceso->modo_apertura);
                
        t_archivo* archivo_buscado_tabla = buscar_en_tabla_de_archivos_abiertos(proceso->nombre_archivo);
        
        //aca vemos si el archivo lo tengo en la TGAA
        if (!existe_en_tabla)
        {
            log_info(kernel_logger, "El archivo no se encuentra en la TGAA. Enviando al filesystem apertura del archivo %s\n", proceso->nombre_archivo);
            //si no lo tengo, le pido al fs que lo abra
            enviar_solicitud_fs(proceso, proceso->nombre_archivo, ABRIR_ARCHIVO, 0, 0, 0);
            bool apertura_terminada = false;
    
            while(!apertura_terminada)
            {
                //recibimos paquete de fs con info
                t_paquete* paquete = recibir_paquete(socket_filesystem);
                void *stream = paquete->buffer->stream;
                int tamanio;
	                            
                if(paquete->codigo_operacion == ARCHIVO_NO_EXISTE)
                {
                    int entero = sacar_entero_de_paquete(&stream);

                    //si el fs me dice que no existe, lo mando a que me lo cree
                    log_info(kernel_logger, "Enviando al filesystem creacion del archivo %s\n", proceso->nombre_archivo);
                    enviar_solicitud_fs(proceso, proceso->nombre_archivo, CREAR_ARCHIVO, 0, 0, 0);

                }
                else if (paquete->codigo_operacion == ARCHIVO_CREADO || paquete->codigo_operacion == ARCHIVO_EXISTE)
                {
                    //ahora mandamos devuelta abrirlo
                    enviar_solicitud_fs(proceso, proceso->nombre_archivo, ABRIR_ARCHIVO, 0, 0, 0);
                }
                else if(paquete->codigo_operacion == ARCHIVO_ABIERTO){
                    
                    //si el fs me dice que abrio el archivo, lo agrego a la tgaa
                    tamanio = sacar_entero_de_paquete(&stream);
                    uint32_t direccion_archivo = sacar_entero_de_paquete(&stream); //direccion inicial (creo que esta bien esto)

                    //poner en tabla de archivos abiertos y guardo el proceso que pidio el lock
                    agregar_archivo_tgaa(proceso, tamanio, direccion_archivo, proceso->pid);
                    
                    apertura_terminada = true;
                }
            }

        }
        else 
        {
            log_info(kernel_logger, "El archivo ya fue abierto y se encuentra en la TGAA\n");

            //si fue abierto, independientemente de si tiene lock de escritura o lectura, si quiero escribir tengo que esperar a que termine el resto
            if (string_equals_ignore_case(proceso->modo_apertura, "W"))
            {
                printf("\n bloqueo el proceso %d porque tengo w y ya fue abierto\n", proceso->pid);
                list_add(archivo_buscado_tabla->cola_solicitudes,(void*)proceso);
                proceso->modo_apertura = "W";
                bloquear_proceso_por_archivo(proceso);
            }
        }

        t_archivo* archivo_encontrado = buscar_en_tabla_de_archivos_abiertos(proceso->nombre_archivo);
        
        //si hay un proceso escribiendo y yo quiero leer, me bloqueo hasta que el otro termine de escribir
        if(string_equals_ignore_case(proceso->modo_apertura, "R") && archivo_encontrado->fcb->lock_escritura)
        {
            liberar_memoria(&proceso->modo_apertura);

            //bloqueamos hasta que termine de escribir el otro proceso
            list_add(archivo_encontrado->cola_solicitudes,(void*)proceso);
            printf("\n bloqueo el proceso %d porque tengo r y ya fue abierto fuera del if\n", proceso->pid);
            
            //proceso->modo_apertura = "R";
            bloquear_proceso_por_archivo(proceso);

        }else if(string_equals_ignore_case(proceso->modo_apertura, "R") && !archivo_encontrado->fcb->lock_escritura)
        {
            //si yo quiero leer y nadie esta escribiendo, lo asigno
            archivo_encontrado->fcb->lock_lectura = true;
            proceso->modo_apertura = "R";
            asignar_archivo_al_proceso(archivo_encontrado, proceso);
            
            //los saco de exec y lo meto a ready
            pthread_mutex_lock(&mutex_ready);
            meter_en_cola(proceso, READY, cola_READY);
            pthread_mutex_unlock(&mutex_ready);
            
        //si quiero escribir y nadie esta leyendo, lo asigno
        }else if(string_equals_ignore_case(proceso->modo_apertura, "W") && !(archivo_encontrado->fcb->lock_escritura) && !(archivo_encontrado->fcb->lock_lectura))
        {
            printf("quiero escribir y nadie esta leyendo ni escribiendo el archivo \n");
            archivo_encontrado->fcb->lock_escritura = true;
            proceso->modo_apertura = "W";
            asignar_archivo_al_proceso(archivo_encontrado, proceso);

            pthread_mutex_lock(&mutex_ready);
            meter_en_cola(proceso, READY, cola_READY);
            pthread_mutex_unlock(&mutex_ready);
            
        //si quiero escribir y alguien esta usando el archivo, lo bloqueo    
        }
        proceso_en_ready();
        break;

    case TRUNCAR_ARCHIVO:
        free(motivo_bloqueo);
        tamanio = proceso->tamanio_archivo;
        
        t_archivo_proceso* archivo = buscar_en_tabla_de_archivos_proceso(proceso, proceso->nombre_archivo);
        
        if(archivo == NULL)
        {
            log_info(kernel_logger, "El proceso no puede realizar operaciones sobre este archivo");
            log_info(kernel_logger, "Finaliza el proceso %d - Motivo: INVALID_RESOURCE\n", proceso->pid);
            proceso_en_exit(proceso);  
        }

        pthread_mutex_lock(&mutex_ready);
        meter_en_cola(proceso, READY, cola_READY);
        pthread_mutex_unlock(&mutex_ready);
        
        enviar_solicitud_fs(proceso, proceso->nombre_archivo, TRUNCAR_ARCHIVO, tamanio, 0, 0);
                
        int respuesta = 0;
        recv(socket_filesystem, &respuesta, sizeof(int),0);
        
        if (respuesta != 1)
        {
            log_error(kernel_logger, "Hubo un error con la respuesta de fs de truncar \n");
        }
        
        break;

    case LEER_ARCHIVO:
        free(motivo_bloqueo);
        direccion_fisica = proceso->direccion_fisica_proceso;

        //en la tabla del proceso guardo en que modo abrio el archivo
        t_archivo_proceso* archivo_para_leer = buscar_en_tabla_de_archivos_proceso(proceso, proceso->nombre_archivo);

        t_archivo* archivo_para_leer_tgaa = buscar_en_tabla_de_archivos_abiertos(proceso->nombre_archivo);

        if(archivo_para_leer == NULL)
        {
            log_info(kernel_logger, "El proceso no puede realizar operaciones sobre este archivo");
            log_info(kernel_logger, "Finaliza el proceso %d - Motivo: INVALID_RESOURCE\n", proceso->pid);
            proceso_en_exit(proceso);   
        }else{

            //si lo abro para escirbir tambien lo puedo leer
            if(string_equals_ignore_case(archivo_para_leer->modo_apertura, "R") || string_equals_ignore_case(archivo_para_leer->modo_apertura, "W") ) 
            {
                log_info(kernel_logger, "PID: %d - Leer Archivo: %s", proceso->pid, proceso->nombre_archivo);
                        
                /*si hay un lock de escritura y NO ES MIO, en cualquiera de los casos voy a bloquear porque no puedo hacer nada
                if(archivo_para_leer->fcb->lock_escritura && (proceso->pid != archivo_para_leer_tgaa->pid_que_me_lockeo))
                {                                
                    //meto al proceso en la cola de bloqueados del archivo
                    list_add(archivo_para_leer_tgaa->cola_solicitudes,(void*)proceso);

                    //lo meto en la lista de los que van a leer
                    printf("\n bloqueo el proceso %d porque tengo r y hay lock de escritura\n", proceso->pid);
                    bloquear_proceso_por_archivo(proceso->nombre_archivo, proceso, "LEER");

                }else{
                    
                    //si no tiene un lock de escritura, lo puedo leer
                    list_add(cola_locks_lectura, (void*)proceso);*/

                if (string_equals_ignore_case(archivo_para_leer->modo_apertura, "R")){
                    archivo_para_leer->fcb->lock_lectura = true;    
                    cola_locks_lectura += 1;
                }

                pthread_mutex_lock(&mutex_ready);
                meter_en_cola(proceso, READY, cola_READY);
                pthread_mutex_unlock(&mutex_ready);

                enviar_solicitud_fs(proceso, proceso->nombre_archivo, LEER_ARCHIVO, 0, 0, direccion_fisica);

                //el proceso se bloquea hasta que el fs me informe la finalizacion de la operacion
                int ok_read = 0;
                recv(socket_filesystem, &ok_read, sizeof(int),0);

                if (ok_read != 1)
                {
                    log_error(kernel_logger, "Hubo un error con la respuesta de fs\n");
                }
            }else{
                log_info(kernel_logger, "El proceso no puede realizar operaciones sobre este archivo");
                log_info(kernel_logger, "Finaliza el proceso %d - Motivo: INVALID_READ\n", proceso->pid);
                proceso_en_exit(proceso);   
            }
        }
        proceso_en_ready();
        break;

    case ESCRIBIR_ARCHIVO:
        free(motivo_bloqueo);
        t_archivo_proceso* archivo_para_escribir = buscar_en_tabla_de_archivos_proceso(proceso, proceso->nombre_archivo);
        if(archivo_para_escribir == NULL)
        {
            log_info(kernel_logger, "El proceso no puede realizar esta operacion sobre este archivo");
            abort();
        }

        t_archivo* archivo_para_escribir_tgaa = buscar_en_tabla_de_archivos_abiertos(proceso->nombre_archivo);

        //si yo abri el archivo en modo de escritura, puedo escribir
        if(string_equals_ignore_case(archivo_para_escribir->modo_apertura, "W"))
        { 
            liberar_memoria(&proceso->modo_apertura);
                   
            t_archivo* archivo_para_escribir = buscar_en_tabla_de_archivos_abiertos(proceso->nombre_archivo);

            //si hay un lock de escritura pero yo puse le lock de escritura, puedo escribir archivo_para_escribir_tgaa->fcb->lock_lectura && 
            if(archivo_para_escribir_tgaa->fcb->lock_lectura)
            {              
                log_info(kernel_logger, "Alguien esta leyendo");  

                //meto al proceso en la cola de bloqueados del archivo
                list_add(archivo_para_escribir->cola_solicitudes,(void*)proceso);
                printf("\n bloqueo el proceso %d porque tengo r y hay lock de escritura\n", proceso->pid);
                //proceso->modo_apertura = "W";
                bloquear_proceso_por_archivo(proceso);

            }else{

                log_info(kernel_logger, "PID: %d - Escribir Archivo: %s", proceso->pid, proceso->nombre_archivo);

                pthread_mutex_lock(&mutex_ready);
                meter_en_cola(proceso, READY, cola_READY);
                pthread_mutex_unlock(&mutex_ready);

                enviar_solicitud_fs(proceso, proceso->nombre_archivo, SOLICITAR_INFO_ARCHIVO_MEMORIA, 0, proceso->puntero, proceso->direccion_fisica_proceso);
                int escribir_ok = 0;
                recv(socket_filesystem, &escribir_ok, sizeof(int),0);

                if(escribir_ok !=1){
                    log_info(kernel_logger, "problema");
                }
            }
            proceso_en_ready();     
        }else{
            log_info(kernel_logger, "El proceso no puede realizar esta operacion sobre este archivo");
            log_info(kernel_logger, "Finaliza el proceso %d - Motivo: INVALID_WRITE\n", proceso->pid);
            proceso_en_exit(proceso);       
        }
        break;

    case BUSCAR_ARCHIVO:
        free(motivo_bloqueo);
        t_archivo_proceso* archivo_buscado = buscar_en_tabla_de_archivos_proceso(proceso, proceso->nombre_archivo);

        archivo_buscado->puntero_posicion = proceso->puntero;
        log_info(kernel_logger, "PID: %d - Actualizar puntero Archivo: %s - Puntero: %d\n",proceso->pid, proceso->nombre_archivo, archivo_buscado->puntero_posicion);
        
        //aca es necesario mandarlo para que siga el mismo proceso
        proceso_en_execute(proceso);
        break;

    case CERRAR_ARCHIVO:
        free(motivo_bloqueo);

        //busco el archivo en la TGAA y en la tabla del proceso para cerrarlo de ambos si es necesario
        t_archivo_proceso* archi = buscar_en_tabla_de_archivos_proceso(proceso, proceso->nombre_archivo);
        t_archivo* archivo_para_cerrar = buscar_en_tabla_de_archivos_abiertos(archi->fcb->nombre_archivo);

        if(archi == NULL){
            log_info(kernel_logger, "El proceso no puede realizar operaciones sobre este archivo");
            log_info(kernel_logger, "Finaliza el proceso %d - Motivo: INVALID_RESOURCE\n", proceso->pid);
            proceso_en_exit(proceso);       
        }

        //si hay procesos esperando por ese archivo
        if(list_size(archivo_para_cerrar->cola_solicitudes) > 0)
        {
            //agarro cada proceso de la lista de espera
            for(int i = 0 ; i < list_size(archivo_para_cerrar->cola_solicitudes) ; i ++)
            {
                t_pcb* siguiente_proceso = (t_pcb*)list_get(archivo_para_cerrar->cola_solicitudes, i);

                //t_archivo_proceso* archivo_del_proceso = buscar_en_tabla_de_archivos_proceso (siguiente_proceso,archivo_para_cerrar->fcb->nombre_archivo);   

                list_remove(archivo_para_cerrar->cola_solicitudes,i);

                if(string_equals_ignore_case(siguiente_proceso->modo_apertura,"R"))
                {
                    asignar_archivo_al_proceso(archivo_para_cerrar, siguiente_proceso);

                    obtener_siguiente_blocked(siguiente_proceso);
                }else{
                    siguiente_proceso->modo_apertura = "W";

                    asignar_archivo_al_proceso(archivo_para_cerrar, siguiente_proceso);

                    obtener_siguiente_blocked(siguiente_proceso);
                }

                //busco el archivo dentro de la tabla de cada proceso
                //t_archivo_proceso* archivo_del_proceso = buscar_en_tabla_de_archivos_proceso (siguiente_proceso,archivo_para_cerrar->fcb->nombre_archivo);   

                /*si abrio el archivo en lectura
                if(string_equals_ignore_case(archivos_del_proceso->modo_apertura, "R"))
                {
                    //mando el proceso a ready
                    obtener_siguiente_blocked(siguiente_proceso);
                }*/
            }
        }

        if(string_equals_ignore_case(archi->modo_apertura, "R"))
        {
            cola_locks_lectura --;

            if(cola_locks_lectura <= 0)
            {
                archivo_para_cerrar->fcb->lock_lectura = false;
            }
        }
        
        list_remove(archivo_para_cerrar->cola_solicitudes,(void*) proceso);
        //saco el archivo de la lista de archivos del proceso
        list_remove_element(proceso->archivos_abiertos, (void*)archi);

        /*si no hay ningun proceso esperando por el archivo, lo saco de la TGAA
        if(list_size(archivo_para_cerrar->cola_solicitudes) <= 0)
        {
            list_remove_element(tabla_global_archivos_abiertos, (void*)archivo_para_cerrar);
        }*/

        pthread_mutex_lock(&mutex_ready);
        meter_en_cola(proceso, READY, cola_READY);
        pthread_mutex_unlock(&mutex_ready);
        
        proceso_en_ready();   

        break;

    default:
        break;
        //free(archivo_encontrado);
    }
}


static int tipo_motivo(char *motivo)
{
    int numero = -1;
    
    if (string_equals_ignore_case(motivo, "F_OPEN"))
    {
        numero = ABRIR_ARCHIVO;

    } else if (string_equals_ignore_case(motivo, "F_CLOSE"))
    {
        numero = CERRAR_ARCHIVO;

    } else if (string_equals_ignore_case(motivo, "F_SEEK"))
    {
        numero = BUSCAR_ARCHIVO;

    } else if (string_equals_ignore_case(motivo, "F_READ"))
    {
        numero = LEER_ARCHIVO;

    } else if (string_equals_ignore_case(motivo, "F_WRITE"))
    {
        numero = ESCRIBIR_ARCHIVO;

    } else if (string_equals_ignore_case(motivo, "F_TRUNCATE"))
    {
        numero = TRUNCAR_ARCHIVO;

    }

    //free(motivo);

    return numero;
}

void enviar_solicitud_fs(t_pcb* proceso, char* nombre_archivo, op_code operacion, int tamanio, uint32_t puntero, uint32_t direccion_fisica)
{
    t_paquete *paquete = crear_paquete(operacion);
    agregar_cadena_a_paquete(paquete, nombre_archivo);
    agregar_entero_a_paquete(paquete, tamanio);
    agregar_entero_sin_signo_a_paquete(paquete, puntero);
    agregar_entero_sin_signo_a_paquete(paquete, direccion_fisica);

    enviar_paquete(paquete, socket_filesystem);
    eliminar_paquete(paquete);
}

//lo saco de exec y lo bloqueo
void bloquear_proceso_por_archivo(t_pcb* proceso)
{
    log_info(kernel_logger, "PID[%d] bloqueado por accion: %s en archivo <%s>\n", proceso->pid, proceso->modo_apertura ,proceso->nombre_archivo);

    pthread_mutex_lock(&mutex_exec);
    list_remove_element(dictionary_int_get(diccionario_colas, EXEC), proceso);
    pthread_mutex_unlock(&mutex_exec);

    pthread_mutex_lock(&mutex_blocked);
    meter_en_cola(proceso, BLOCKED, cola_BLOCKED);
    pthread_mutex_unlock(&mutex_blocked);

}
//============================================= TABLA ARCHIVOS ABIERTOS DEL PROCESO =======================================

void asignar_archivo_al_proceso(t_archivo* archivo,t_pcb* proceso)
{
    //suponemos que la direccino que nos mandan es la direccino inicial apenas se crea
    //la manera de encontrar va a ser con el pid

    t_archivo_proceso* nuevo_archivo = malloc(sizeof(t_archivo_proceso));
    nuevo_archivo->fcb = malloc(sizeof(fcb_proceso));
    uint32_t direccion_archivo_proceso = (uint32_t)nuevo_archivo;
    
    //guardo el fcb que tiene el archivo que quiero agregar a mi proceso, que lo cree mas arriba
    nuevo_archivo->fcb = archivo->fcb;
    nuevo_archivo->puntero_posicion = direccion_archivo_proceso;

    //el modo de apertura es como yo abri al archivo
    //nuevo_archivo->modo_apertura =  strdup(modo_apertura);
    nuevo_archivo->modo_apertura = strdup(proceso->modo_apertura); //REVISAR

    //agregar a la tabla de archivos abiertos del proceso
    list_add(proceso->archivos_abiertos,(void*)nuevo_archivo);

    log_info(kernel_logger, "Se guardo el archivo %s en la tabla de archivos del proceso PID [%d]\n",nuevo_archivo->fcb->nombre_archivo ,proceso->pid);

    liberar_memoria(&proceso->modo_apertura);

}

t_archivo_proceso* buscar_en_tabla_de_archivos_proceso(t_pcb* proceso, char* nombre_a_buscar)
{
    if(proceso->archivos_abiertos != NULL && list_size(proceso->archivos_abiertos) > 0)
    {
        for (int i = 0; i < list_size(proceso->archivos_abiertos); i++)
        {
            t_archivo_proceso* archivo = (t_archivo_proceso*)list_get(proceso->archivos_abiertos, i);

            if (strcmp(nombre_a_buscar,archivo->fcb->nombre_archivo) == 0)
            {
                return archivo;
            }
        }
    }
    return NULL;
}

//============================================= TABLA GLOBAL DE ARCHIVOS ABIERTOS =======================================

void agregar_archivo_tgaa(t_pcb* proceso, int tamanio, uint32_t direccion, int pid)
{
    t_archivo* archivo_nuevo = malloc(sizeof(t_archivo));
    archivo_nuevo->fcb = malloc(sizeof(fcb_proceso));

    archivo_nuevo->fcb->nombre_archivo = strdup(proceso->nombre_archivo);
    archivo_nuevo->fcb->tamanio = tamanio;
    archivo_nuevo->fcb->lock_escritura = false;
    archivo_nuevo->fcb->lock_lectura = false;
    archivo_nuevo->fcb->bloque_inicial = 1; 
    archivo_nuevo->puntero_posicion = direccion;
    archivo_nuevo->cola_solicitudes = list_create();
    archivo_nuevo->pid_que_me_lockeo = pid;
    
    list_add(tabla_global_archivos_abiertos, (void*)archivo_nuevo);

    existe_en_tabla = true;
}


t_archivo* buscar_en_tabla_de_archivos_abiertos(char* nombre_a_buscar)
{
    if(tabla_global_archivos_abiertos != NULL && list_size(tabla_global_archivos_abiertos) > 0)
    {
        for (int i = 0; i < list_size(tabla_global_archivos_abiertos); i++)
        {
            t_archivo* archivo_de_tabla = (t_archivo*)list_get(tabla_global_archivos_abiertos, i);

            if (strcmp(nombre_a_buscar,archivo_de_tabla->fcb->nombre_archivo) == 0)
            {
                log_info(kernel_logger, "Se encontro el archivo %s  en la TGAA \n", nombre_a_buscar);
                existe_en_tabla = true;
                return archivo_de_tabla;
            }
        }
    }else{
        existe_en_tabla = false;
    }
    return NULL;
}

