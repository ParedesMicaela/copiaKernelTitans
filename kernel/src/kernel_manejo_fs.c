#include "kernel.h"

sem_t instruccion_tipo_archivo;

t_list *tabla_global_archivos_abiertos;
bool existe_en_tabla = true;
 
t_list *cola_locks_lectura; //tiene a todos los archivos que se estan leyendo
op_code devuelto_por=-1;

static int tipo_motivo(char *motivo);
//=============================================================================================================

void iniciar_tabla_archivos_abiertos()
{
    tabla_global_archivos_abiertos = list_create();
    cola_locks_lectura =  list_create();
}

void atender_peticiones_al_fs(t_pcb* proceso)
{
    char* nombre_archivo = NULL;
    char* modo_apertura = NULL;
    int tamanio = 0 ;
    int puntero = 0;
    uint32_t direccion_fisica = 0;

    switch(tipo_motivo(proceso->motivo_bloqueo)){  

    case ABRIR_ARCHIVO:
        nombre_archivo =  proceso->nombre_archivo;
        modo_apertura = proceso->modo_apertura;
        log_info(kernel_logger, "PID: %d - Abrir Archivo: %s", proceso->pid, nombre_archivo);
                
        t_archivo* archivo_buscado_tabla = buscar_en_tabla_de_archivos_abiertos(nombre_archivo);
        
        //aca vemos si el archivo lo tengo en la TGAA
        if (!existe_en_tabla)
        {
            log_info(kernel_logger, "El archivo no se encuentra en la TGAA. Enviando al filesystem apertura del archivo %s\n", nombre_archivo);
            //si no lo tengo, le pido al fs que lo abra
            enviar_solicitud_fs(nombre_archivo, ABRIR_ARCHIVO, 0, 0, 0);
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
                    enviar_solicitud_fs(nombre_archivo, CREAR_ARCHIVO, 0, 0, 0);
                    log_info(kernel_logger, "Enviando al filesystem creacion del archivo %s\n", nombre_archivo);

                }
                else if (paquete->codigo_operacion == ARCHIVO_CREADO || paquete->codigo_operacion == ARCHIVO_EXISTE)
                {
                    //ahora mandamos devuelta abrirlo
                    enviar_solicitud_fs(nombre_archivo, ABRIR_ARCHIVO, 0, 0, 0);
                }
                else if(paquete->codigo_operacion == ARCHIVO_ABIERTO){
                    
                    //si el fs me dice que abrio el archivo, lo agrego a la tgaa
                    tamanio = sacar_entero_de_paquete(&stream);
                    uint32_t direccion_archivo = sacar_entero_de_paquete(&stream); //direccion inicial (creo que esta bien esto)
                    printf("recibimos que el archivo ya existe\n");
                    //poner en tabla de archivos abiertos
                    agregar_archivo_tgaa(nombre_archivo, tamanio, direccion_archivo);
                    
                    apertura_terminada = true;
                }
            }
        }
        else if (existe_en_tabla)
        {
            log_info(kernel_logger, "El archivo ya fue abierto y se encuentra en la TGAA\n");

            //si fue abierto, independientemente de si tiene lock de escritura o lectura, si quiero escribir tengo que esperar a que termine el resto
            if (string_equals_ignore_case(modo_apertura, "w") == 0)
            {
                pthread_mutex_lock(&mutex_exec);
                list_remove_element(dictionary_int_get(diccionario_colas, EXEC), proceso);
                pthread_mutex_unlock(&mutex_exec);

                pthread_mutex_lock(&mutex_blocked);
                meter_en_cola(proceso, BLOCKED, cola_BLOCKED);
                pthread_mutex_unlock(&mutex_blocked);

                log_info(kernel_logger, "PID[%d] bloqueado por %s\n", proceso->pid, nombre_archivo);
            }
        }

        t_archivo* archivo_encontrado = buscar_en_tabla_de_archivos_abiertos(nombre_archivo);
        
        //si hay un proceso escribiendo y yo quiere leer, lo bloqueo hasta que el otro termine de escribir
        if(string_equals_ignore_case(modo_apertura, "r") == 0 && archivo_encontrado->fcb->lock_escritura)
        {
            //bloqueamos hasta que termine de escribir el otro proceso
            pthread_mutex_lock(&mutex_exec);
            list_remove_element(dictionary_int_get(diccionario_colas, EXEC), proceso);
            pthread_mutex_unlock(&mutex_exec);

            pthread_mutex_lock(&mutex_blocked);
            meter_en_cola(proceso, BLOCKED, cola_BLOCKED);
            pthread_mutex_unlock(&mutex_blocked);

            log_info(kernel_logger, "PID[%d] bloqueado por %s\n", proceso->pid, archivo_encontrado->fcb->nombre_archivo);

        }else{
            asignar_archivo_al_proceso(archivo_encontrado, proceso, modo_apertura);
            //consola_proceso_estado();
            proceso_en_execute(proceso);    
        }
        
        break;

    case TRUNCAR_ARCHIVO:
        nombre_archivo = proceso->nombre_archivo;
        tamanio = proceso->tamanio_archivo;
        
        t_archivo_proceso* archivo = buscar_en_tabla_de_archivos_proceso(proceso, nombre_archivo);
        
        if(archivo == NULL)
        {
            log_info(kernel_logger, "El proceso no puede realizar operaciones sobre este archivo");
            log_info(kernel_logger, "Finaliza el proceso %d - Motivo: INVALID_RESOURCE\n", proceso->pid);
            proceso_en_exit(proceso);  
        }
        
        enviar_solicitud_fs(nombre_archivo, TRUNCAR_ARCHIVO, tamanio, 0, 0);
        
        log_info(kernel_logger, "Espero a que me llegue que trunco bien\n");
        
        bool truncando;
        
        while(truncando)
        {
            t_paquete* paquete = recibir_paquete(socket_filesystem);
            void *stream = paquete->buffer->stream;
            
            if(paquete->codigo_operacion == ARCHIVO_TRUNCADO)
            {
                truncando=false;
                proceso_en_execute(proceso);
            }
            else
            {
                perror("no me llego que se trunco, que pingo es esto");
                abort();
            }
        }
        break;
    case LEER_ARCHIVO:

        nombre_archivo =  proceso->nombre_archivo;
        puntero = proceso->posicion;
        direccion_fisica = proceso->direccion_fisica_proceso;

        log_info(kernel_logger, "PID: %d - Leer Archivo: %s", proceso->pid, nombre_archivo);
                
        t_archivo* archivo_para_leer = buscar_en_tabla_de_archivos_abiertos(nombre_archivo);

        //si hay un lock de escritura, en cualquiera de los casos voy a bloquear porque no puedo hacer nada
        if(archivo_para_leer->fcb->lock_escritura == true)
        {                                     
            //meto al proceso en la cola de bloqueados del archivo
            list_add(archivo_para_leer->cola_solicitudes,(void*)proceso);

        }else{
            
            /*
            //si no existe lock de escritura veo si existe un lock de lectura
            if(archivo_para_leer->fcb->lock_lectura == true)
            {
                //lo meto a la lista de los que van a leer
                list_add(cola_locks_lectura, (void*)proceso);
            
            }else{

                //si no hay una lista de lectura porque nadie lo esta leyendo, creo la lista y agrego  el proceso como participante
                archivo_para_leer->fcb->lock_lectura = true;
                list_add(cola_locks_lectura, (void*)proceso);
            }
            */
            list_add(cola_locks_lectura, (void*)proceso);

            enviar_solicitud_fs(nombre_archivo, LEER_ARCHIVO, 0, 0, direccion_fisica);

            //el proceso se bloquea hasta que el fs me informe la finalizacion de la operacion
            int respuesta = 0;
            recv(socket_filesystem, &respuesta, sizeof(int),0);

            if (respuesta != 1)
            {
                log_error(kernel_logger, "Hubo un error con la respuesta de fs\n");
            }

            proceso_en_execute(proceso);
        }
        break;

    case ESCRIBIR_ARCHIVO:
        nombre_archivo =  proceso->nombre_archivo;
        puntero = proceso->posicion;
        direccion_fisica = proceso->direccion_fisica_proceso;

        log_info(kernel_logger, "PID: %d - Escribir Archivo: %s", proceso->pid, nombre_archivo);
                
        t_archivo* archivo_para_escribir = buscar_en_tabla_de_archivos_abiertos(nombre_archivo);

        //si hay un lock de escritura, en cualquiera de los casos voy a bloquear porque no puedo hacer nada
        if(archivo_para_escribir->fcb->lock_escritura == true)
        {                                     
            //meto al proceso en la cola de bloqueados del archivo
            list_add(archivo_para_escribir->cola_solicitudes,(void*)proceso);
        }else{

            archivo_para_escribir->fcb->lock_escritura = true;
            enviar_solicitud_fs(nombre_archivo, ESCRIBIR_ARCHIVO, 0, puntero, direccion_fisica);
        }
        break;

    case BUSCAR_ARCHIVO:
        nombre_archivo =  proceso->nombre_archivo;
        puntero = proceso->posicion;

        t_archivo_proceso* archivo_buscado = buscar_en_tabla_de_archivos_proceso(proceso, nombre_archivo);
        
        if(archivo_buscado == NULL){
            log_info(kernel_logger, "El proceso no puede realizar operaciones sobre este archivo");
            log_info(kernel_logger, "Finaliza el proceso %d - Motivo: INVALID_RESOURCE\n", proceso->pid);
            proceso_en_exit(proceso);       
        }

        archivo_buscado->puntero_posicion = (uint32_t)puntero;
        log_info(kernel_logger, "PID: %d - Actualizar puntero Archivo: %s - Puntero: %d\n",proceso->pid, nombre_archivo, archivo_buscado->puntero_posicion);
        proceso_en_execute(proceso);
        break;

    case CERRAR_ARCHIVO:
        nombre_archivo =  proceso->nombre_archivo;

        //busco el archivo en la TGAA y en la tabla del proceso para cerrarlo de ambos si es necesario
        t_archivo_proceso* archi = buscar_en_tabla_de_archivos_proceso(proceso, nombre_archivo);
        t_archivo* archivo_para_cerrar = buscar_en_tabla_de_archivos_abiertos(nombre_archivo);

        if(archi == NULL){
            log_info(kernel_logger, "El proceso no puede realizar operaciones sobre este archivo");
            log_info(kernel_logger, "Finaliza el proceso %d - Motivo: INVALID_RESOURCE\n", proceso->pid);
            proceso_en_exit(proceso);       
        }

        if(string_equals_ignore_case(archi->modo_apertura, "r") == 0)
        {
            if(cola_locks_lectura != NULL)
            {
                //elimino de la cola de lectura al proceso que cerro el archivo
                list_remove_element(cola_locks_lectura, (void *)proceso);

                if(list_size(cola_locks_lectura) < 0)
                {
                    list_destroy(cola_locks_lectura);

                    //si nadie lo esta leyendo, busco el proceso que esta esperando a escribirlo
                    if(list_size(archivo_para_cerrar->cola_solicitudes) > 0)
                    {
                        t_pcb* siguiente_proceso = (t_pcb*)list_get(archivo_para_cerrar->cola_solicitudes, 0);
                        proceso_en_execute(siguiente_proceso);
                    }else{

                        //si nadie lo esta leyendo y nadie esta esperando hacer algo con el archivo, lo cierro de la TGAA
                        list_remove(tabla_global_archivos_abiertos, (void*)archivo_para_cerrar);
                    }
                }
            }
        }

        if(string_equals_ignore_case(archi->modo_apertura, "w") == 0)
        {
            if(list_size(archivo_para_cerrar->cola_solicitudes) > 0)
            {
                t_pcb* siguiente_proceso = (t_pcb*)list_get(archivo_para_cerrar->cola_solicitudes, 0);
                proceso_en_execute(siguiente_proceso);
            }
        }

        list_remove_element(proceso->archivos_abiertos, (void*)archi);

        //si no hay ningun proceso esperando por el archivo, lo saco de la TGAA
        if(list_size(archivo_para_cerrar->cola_solicitudes) <= 0)
        {
            list_remove_element(tabla_global_archivos_abiertos, (void*)archivo_para_cerrar);
        }
        proceso_en_execute(proceso);
        break;

    default:
        log_error(kernel_logger, "Error en el motivo de bloqueo\n");
        break;
        free(archivo_encontrado);
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

    }else
    {
        log_info(kernel_logger, "no encontre el motivo %d\n", numero);
        abort();
    }
    return numero;
}

void enviar_solicitud_fs(char* nombre_archivo, op_code operacion, int tamanio, uint32_t posicion, uint32_t direccion_fisica)
{
    t_paquete *paquete = crear_paquete(operacion);
    agregar_cadena_a_paquete(paquete, nombre_archivo);
    //agregar_cadena_a_paquete(paquete, archivo->fcb->bloque_inicial);
    agregar_entero_a_paquete(paquete, tamanio);
    agregar_entero_a_paquete(paquete, posicion);
    agregar_entero_sin_signo_a_paquete(paquete, direccion_fisica);
    enviar_paquete(paquete, socket_filesystem);
    eliminar_paquete(paquete);
}

//============================================= TABLA ARCHIVOS ABIERTOS DEL PROCESO =======================================

void asignar_archivo_al_proceso(t_archivo* archivo,t_pcb* proceso, char* modo_apertura)
{
    //suponemos que la direccino que nos mandan es la direccino inicial apenas se crea
    //la manera de encontrar va a ser con el pid

    t_archivo_proceso* nuevo_archivo = malloc(sizeof(t_archivo_proceso));
    nuevo_archivo->fcb = malloc(sizeof(fcb_proceso));
    uint32_t direccion_archivo_proceso = (uint32_t)nuevo_archivo;
    
    //guardo el fcb que tiene el archivo que quiero agregar a mi proceso, que lo cree mas arriba
    nuevo_archivo->fcb = archivo->fcb;
    nuevo_archivo->puntero_posicion = direccion_archivo_proceso;
    nuevo_archivo->modo_apertura =  strdup(modo_apertura);

    //agregar a la tabla de archivos abiertos del proceso
    list_add(proceso->archivos_abiertos,(void*)nuevo_archivo);

    log_info(kernel_logger, "Se guardo el archivo %s en la tabla de archivos del proceso PID [%d]\n",nuevo_archivo->fcb->nombre_archivo ,proceso->pid);
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
                log_info(kernel_logger, "Se encontro el archivo %s en la tabla del proceso \n", nombre_a_buscar);
                return archivo;
            }
        }
    }
    return NULL;
}

//============================================= TABLA GLOBAL DE ARCHIVOS ABIERTOS =======================================

void agregar_archivo_tgaa(char* nombre_archivo, int tamanio, uint32_t direccion)
{
    t_archivo* archivo_nuevo = malloc(sizeof(t_archivo));
    archivo_nuevo->fcb = malloc(sizeof(fcb_proceso));

    archivo_nuevo->fcb->nombre_archivo = strdup(nombre_archivo);
    //archivo_nuevo->fcb->nombre_archivo = nombre_archivo;
    archivo_nuevo->fcb->tamanio = tamanio;
    archivo_nuevo->fcb->lock_escritura = false;
    archivo_nuevo->fcb->lock_lectura = false;
    archivo_nuevo->fcb->bloque_inicial = 1; 
    archivo_nuevo->puntero_posicion = direccion;
    archivo_nuevo->cola_solicitudes = list_create();

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

