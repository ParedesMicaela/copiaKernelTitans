#include "kernel.h"

sem_t instruccion_tipo_archivo;

t_list *tabla_global_archivos_abiertos;
bool existe_en_tabla = true;
 
t_list *cola_locks_lectura; //tiene a todos los archivos que se estan leyendo
t_list *cola_locks_escritura_bloqueados; // aca se guardan los archivos que quieren escribirse
op_code devuelto_por=-1;


static int tipo_motivo(char *motivo);
//=============================================================================================================

void iniciar_tabla_archivos_abiertos()
{
    tabla_global_archivos_abiertos = list_create();
}

void atender_peticiones_al_fs(t_pcb* proceso)
{
    while(1)
    {
        printf("Espero recibir paquete de CPU respecto a archivos\n");

        switch(tipo_motivo(proceso->motivo_bloqueo)){    
        case ABRIR_ARCHIVO:

            char* nombre_archivo =  proceso->nombre_archivo;
            char* modo_apertura = proceso->modo_apertura;
            log_info(kernel_logger, "PID: %d - Abrir Archivo: %s", proceso->pid, nombre_archivo);
                    
            t_archivo* archivo_encontrado = buscar_en_tabla_de_archivos_abiertos(nombre_archivo);

            //aca vemos si el archivo lo tengo en la TGAA
            if (!existe_en_tabla)
            {
                printf("no esta en tabla\n");
                //si no lo tengo, le pido al fs que lo abra
                enviar_solicitud_fs(nombre_archivo, ABRIR_ARCHIVO);

                //recibimos paquete de fs con info
                t_paquete* paquete = recibir_paquete(socket_filesystem);
                void *stream = paquete->buffer->stream;
                int tamanio;
                printf("Recibimos paquete de fs");

                if(paquete->codigo_operacion == ARCHIVO_NO_EXISTE)
                {
                    //si el fs me dice que no existe, lo mando a que me lo cree
                    enviar_solicitud_fs(nombre_archivo, CREAR_ARCHIVO);

                }else if (paquete->codigo_operacion == ARCHIVO_CREADO || paquete->codigo_operacion == ARCHIVO_EXISTE){

                    printf("el fs ya me mando algo bueno\n");
                    //si el fs me dice que existe el archivo, lo agrego a la tgaa
                    tamanio = sacar_entero_de_paquete(&stream);
                    agregar_archivo_tgaa(nombre_archivo, tamanio);
                    
                    t_archivo* archivo_encontrado = buscar_en_tabla_de_archivos_abiertos(nombre_archivo);

                    // poner en tabla de archivos abiertos
                    asignar_archivo_al_proceso(archivo_encontrado);

                }else{
                    log_error(kernel_logger, "FS no me creo el archivo :c\n");
                }

            }else{
                
                //si el archivo ya fue abierto hago cosas dependiendo el modo de apertura
                if(string_equals_ignore_case(modo_apertura, "r"))
                {
                    
                }else if(string_equals_ignore_case(modo_apertura, "w")){

                }

            }
            break;
        case TRUNCAR_ARCHIVO:
            break;
        case LEER_ARCHIVO:
            break;
        case ESCRIBIR_ARCHIVO:
            break;
        case BUSCAR_ARCHIVO:
            break;
        case CERRAR_ARCHIVO:
            break;
        default:
            log_error(kernel_logger, "Error en el motivo de bloqueo");
            break;
            free(archivo_encontrado);
        }
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
        numero = F_CLOSE;

    } else if (string_equals_ignore_case(motivo, "F_SEEK"))
    {
        numero = F_SEEK;

    } else if (string_equals_ignore_case(motivo, "F_READ"))
    {
        numero = F_READ;

    } else if (string_equals_ignore_case(motivo, "F_WRITE"))
    {
        numero = F_WRITE;

    } else if (string_equals_ignore_case(motivo, "F_TRUNCATE"))
    {
        numero = F_TRUNCATE;

    }else
    {
        log_info(kernel_logger, "no encontre el motivo %d\n", numero);
        abort();
    }
    return numero;
}

void asignar_archivo_al_proceso(t_archivo* archivo,t_pcb* proceso){
    //agregar a la tabla del proceso
    //tabla de proceso va a tener archivos respecto de cada proceso
    //la manera de encontrar va a ser con el pid
}

void agregar_archivo_tgaa(char* nombre_archivo, int tamanio)
{
    t_archivo* archivo_nuevo = malloc(sizeof(t_archivo));
    archivo_nuevo->nombre_archivo = nombre_archivo;
    archivo_nuevo->tamanio = tamanio;
    archivo_nuevo->lock_escritura = false;
    archivo_nuevo->cola_solicitudes = list_create();
    //archivo_nuevo->modo_apertura

    list_add(tabla_global_archivos_abiertos, (void*)archivo_nuevo);
    existe_en_tabla = true;
}

void enviar_solicitud_fs(char* nombre_arch, op_code operacion)
{
    t_paquete *paquete = crear_paquete(operacion);
    agregar_cadena_a_paquete(paquete, nombre_arch);
    enviar_paquete(paquete, socket_filesystem);
    eliminar_paquete(paquete);
}

t_archivo* buscar_en_tabla_de_archivos_abiertos(char* nombre_a_buscar)
{
    if(tabla_global_archivos_abiertos != NULL && list_size(tabla_global_archivos_abiertos) > 0)
    {
        for (int i = 0; i < list_size(tabla_global_archivos_abiertos); i++)
        {
            t_archivo* archivo_de_tabla = (t_archivo*)list_get(tabla_global_archivos_abiertos, i);

            if (strcmp(nombre_a_buscar,archivo_de_tabla->nombre_archivo) == 0)
            {
                log_info(kernel_logger, "El archivo < %s > a esta abierto", nombre_a_buscar);
                existe_en_tabla = true;
                //printf("encontramos archivo");
                return archivo_de_tabla;
            }
        }
    }else{
        existe_en_tabla = false;
        //printf("no encontramos le archivo");
    }
    return NULL;
}


/*

void fopen_kernel_filesystem()
{
    devuelto_por = ABRIR_ARCHIVO;
    sem_post(&instruccion_tipo_archivo);
}

void fclose_kernel_filesystem()
{
    devuelto_por = CERRAR_ARCHIVO;    
    sem_post(&instruccion_tipo_archivo);
}

void fseek_kernel_filesystem()
{
    devuelto_por = POSICIONARSE_ARCHIVO;    
    sem_post(&instruccion_tipo_archivo);
}

void fread_kernel_filesystem()
{
    devuelto_por = LEER_ARCHIVO;    
    sem_post(&instruccion_tipo_archivo);
}

void fwrite_kernel_filesystem()
{
    devuelto_por = ESCRIBIR_ARCHIVO;    
    sem_post(&instruccion_tipo_archivo);
}

void ftruncate_kernel_filesystem()
{
    devuelto_por = TRUNCAR_ARCHIVO;    
    sem_post(&instruccion_tipo_archivo);
}






int abrirArchivoKernel(pcb* proceso, char* instruccion)
{
	char** parsed = string_split(instruccion, " "); //Partes de la instruccion actual
	bool archivoExisteEnTabla = false;
	Archivo* archivoActual;
	int punteroOriginal;
	//verificar si el archivo esta en la tabla global.
	for (int i = 0; i < list_size(archivosAbiertos); i++)
	{
	    archivoActual = (Archivo*)list_get(archivosAbiertos, i);
	    if (strcmp(archivoActual->nombreDeArchivo, parsed[1]) == 0)
	    {
	        archivoExisteEnTabla = true;
	        break;
	    }
	}
	if (!archivoExisteEnTabla)
	{
		//agrega el archivo a la lista global
		archivoActual = malloc(sizeof(Archivo));
		archivoActual->nombreDeArchivo=malloc(strlen(parsed[1])+1);
		strcpy(archivoActual->nombreDeArchivo,parsed[1]);
		archivoActual->procesosBloqueados = queue_create();
		archivoActual->puntero=0;
		list_add(archivosAbiertos, archivoActual);
		//agregar el archivo a la lista de archivos abiertos del proceso
		list_add(proceso->tabla_global_archivos_abiertos,archivoActual);
		log_info(logger, "PID: %d - Abrir Archivo: %s", proceso->pid, parsed[1]);
		return 1;
	}
	else
	{
		//agregar el proceso a la lista de procesos bloqueados por este archivo
		archivoActual->procesosBloqueados = queue_create();
		punteroOriginal=archivoActual->puntero;
		archivoActual->puntero=0;
		//agregar el archivo a la lista de archivos abiertos del proceso
		list_add(proceso->tabla_global_archivos_abiertos,archivoActual);
		archivoActual->puntero=punteroOriginal;
		exec_a_block();
		log_warning(logger, "PID: %d - Bloqueado porque archivo %s ya esta abierto", proceso->pid, archivoActual->nombreDeArchivo);
		return 0;
	}
}
*/

/*
Las acciones que se pueden realizar en cuanto a los archivos van a venir por parte de la CPU
como llamadas por medio de las funciones correspondientes a archivos.
//paquete con el tipo de operacion y un switch

El Kernel deberá implementar una tabla global de archivos abiertos,
para gestionar los mismos como si fueran un recurso de una única instancia.
//estructura y declarada como lista

Las operaciones de Filesystem se dividirán en dos tipos de locks obligatorios:
    -lock de lectura (R)
        -se resuelve sin bloquear al proceso si solo si no existe lock de escritura activo
        -otros pueden leer si no hay nada escribiendo
        -si hay lock de escritura se manda a cola de bloqueados
    -lock de escritura (W)
        -bloquea a los otros lock y escribe en el archivo
        -se bloquea si hay algo ya escribiendo y se manda a cola de bloqueados
        
-Locks se hacen en en la función F_OPEN y finalizarán con la función F_CLOSE

-Se tendrá un único lock de lectura (creo que es un hilo) compartido por todos los F_OPEN en modo lectura)
    -se resolverá sin ser bloqueado siempre y cuando no exista activamente un lock de escritura.

-Un lock exclusivo por cada pedido de apertura que llegue en modo escritura (serie de hilos)
Al momento en el que se active el lock de escritura:
    -encolar locks de lectura o escritura que se requieran para dicho archivo.
    -lock se bloqueará hasta que se resuelvan:
        --pedidos que participen en lock de archivo que se encontraban activos.

Cada proceso va a contar con su propio puntero:
    -le permitirá desplazarse por el contenido del archivo utilizando F_SEEK
    -luego, si el tipo de lockeo lo permite:
        --efectuar una lectura (F_READ) o escritura (F_WRITE) sobre el mismo 


Para ejecutar estas funciones va a recibir:
    -el Contexto de Ejecución de la CPU
    -la función como motivo.
        --El accionar de cada función va a ser el siguiente:
        --//funciones de "kernel_manejo_fs_funciones.c"
*/        