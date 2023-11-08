
#include "kernel.h"

sem_t instruccion_tipo_archivo;

//aca implementamos la tabla de archivos globales
typedef struct archivo {
    FILE *ARCHIVO;          //direccion de memoria del archivo      
    char *nombre_archivo;   //nombre (identificador)
    int *puntero;           
} archivo;

typedef struct lock_escritura {
    bool estado;        //con esto sabemos si lo estan escribiendo
    archivo *archivo;   //con esto sabemos a que archivo estan escribiendo
} lock_escritura;


/*
    //char tipo_apertura;   //esto puede ser que no sea necesario, pero como es de archivos abiertos capaz si
    //int tamanio;          //tamanio en memoria... necesario?? 
    //recursos asignados??
    //algo mas??
    //ponerlo en kernel.h cuando termines
*/

//archivo *lista_archivos = malloc(sizeof(tabla_archivos)); es dinamica???
t_list *archivos_abiertos; // o es local??
 
t_list *cola_locks_lectura; //tiene a todos los archivos que se estan leyendo
t_list *cola_locks_escritura_bloqueados; // aca se guardan los archivos que quieren escribirse

/*
archivo *archivo_nuevo = malloc(sizeof(archivo));
archivo_nuevo->nombre_archivo = nombre_archivo_devuelto;
archivo_nuevo->ARCHIVO = malloc(sizeof(FILE));          //direccion de memoria del archivo      
char *nombre_archivo;                                   //nombre (identificador)
bool lockeado;                                          //para ver si esta lockeado o no (al pedo, porque creo que lo ponemos en una cola y listo)
archivo *siguiente = NULL;
list_add(archivos_abiertos, archivo_nuevo);
*/
op_code devuelto_por=-1;

void atender_peticiones_al_fs()
{
    
    //while(1) creo que esto no va porque creamos un hilo por cada peticion
    //{
        printf("\n\nEspero recibir paquete de CPU respecto a archivos\n\n");

        sem_wait(&instruccion_tipo_archivo);
        
        //ahora sacamos todo de la estructura del kernel.h, la que llenamos en kernel plani.c
        //faltara algo??
        
        //int pid_devuelto = necesitamos pid???
        int program_counter = contexto_ejecucion_manejo_archivos.program_counter;
        int AX = contexto_ejecucion_manejo_archivos.registros_cpu.AX;
        int BX = contexto_ejecucion_manejo_archivos.registros_cpu.BX;
        int CX = contexto_ejecucion_manejo_archivos.registros_cpu.CX;
        int DX = contexto_ejecucion_manejo_archivos.registros_cpu.DX;
        char* motivo = contexto_ejecucion_manejo_archivos.motivo_de_devolucion;
        char** datos = contexto_ejecucion_manejo_archivos.datos_instruccion;
        printf("\n\nSacamos piola los datos\n\n");  
        
        
        //int recurso = contexto_ejecucion_manejo_archivos->recursos_asignados->nombre_recurso; esto creo que es alpedo aca
        
        switch(devuelto_por)
        {
            
            case ABRIR_ARCHIVO:
            
                //fopen archivo modo
                char** datos = contexto_ejecucion_manejo_archivos.datos_instruccion;
                char* nombre_archivo =  datos[0];
                char* modo_apertura = datos[1];
                
                printf("\n\nKernel recibio paquete de CPU para abrir archivo\n\n");

                //vemos si existe en la tabla de archivos abiertos
                bool existe_en_tabla;
                
                existe_en_tabla = buscar_en_tabla_de_archivos_abiertos(nombre_archivo);
                //printf("\n\nBuscamos en la tabla\n\n");
                if (!existe_en_tabla)
                {
                    //no existe en tabla de archivos abiertos
                    printf("\n\nNo existe en la tabla, mandamos paquete a fs\n\n");
                    //mandamos paquete al fs para que abra el archivo
                    t_paquete *paquete = crear_paquete(ABRIR_ARCHIVO);
                    agregar_cadena_a_paquete(paquete, nombre_archivo);
                    agregar_cadena_a_paquete(paquete, modo_apertura);
                    enviar_paquete(paquete, socket_filesystem);
                    eliminar_paquete(paquete);
                    printf("\n\nEnviamos paquete a fs\n\n");

                    //recibimos paquete de fs con info
                    paquete = recibir_paquete(socket_filesystem);
                    void *stream = paquete->buffer->stream;
                    printf("\n\nRecibimos paquete de fs\n\n");

                    //bool archivo_creado = false; //esto lo devuelve el fs
                    //while(!archivo_creado) //capaz esto hay que sacarlo porque puede que le fs no pueda crear archivo
                    //{
                        switch (paquete->codigo_operacion)
                        {
                            case ARCHIVO_NO_EXISTE:
                            {
                                //mandamos a crear
                                printf("\n\nEl archivo no existe en el fs, mandamos a crear\n\n");
                                t_paquete *paquete = crear_paquete(CREAR_ARCHIVO);
                                agregar_cadena_a_paquete(paquete, nombre_archivo);
                                enviar_paquete(paquete, socket_filesystem);
                                eliminar_paquete(paquete);
                                printf("\n\nMandamos a crear a fs\n\n");

                                //recibimos paquete de fs con info
                                paquete = recibir_paquete(socket_filesystem);
                                void *stream = paquete->buffer->stream;
                                printf("\n\nRecibimos respuesta respecto creacino de archivo en fs\n\n");

                                if (paquete->codigo_operacion==ARCHIVO_CREADO)
                                {
                                    //archivo_creado = true;
                                    printf("\n\nLo creo, ahora lo metemos en tabla\n\n");
                                    //crear archivo para la tabla de archivos abiertos
                                    archivo *archivo_nuevo = malloc(sizeof(archivo));
                                    archivo_nuevo->nombre_archivo = nombre_archivo;
                                    archivo_nuevo->ARCHIVO = malloc(sizeof(FILE));           
                                    archivo_nuevo->puntero = archivo_nuevo->ARCHIVO; //esta bien esto? la 1er direccion de memoria

                                    //añadir a la tabla de archivos abiertos
                                    list_add(archivos_abiertos, archivo_nuevo);

                                    printf("se creo el archivo y se agrego a la tabla de archivos abiertos");

                                    //avisar a cpu???
                                } else printf("error creando en fs");
                                break;
                            }
                        }
                    //}
                }
            break;
                 
            case TRUNCAR_ARCHIVO: // falta hacer todo bien
                
                printf("\n\nKernel recibio paquete de CPU para truncar archivo\n\n");

                //fopen archivo modo
                //nombre_archivo  = sacar_cadena_de_paquete(&stream);
                //char* tamanio  = sacar_cadena_de_paquete(&stream);
                //eliminar_paquete(paquete);
                
                //vemos si existe en la tabla de archivos abiertos
                existe_en_tabla = buscar_en_tabla_de_archivos_abiertos(nombre_archivo);
                printf("\n\nBuscamos bien en tabla\n\n");
                //bool esta_lockeado_escritura;

                if (existe_en_tabla)
                {
                    //si se esta escribiendo esperamos a que termine, mandar a cola de lockeados
                    //si no se esta escribiendo lockeamos este archivo (cambiar bool o meter en cola)
                }
                else //si no existe
                {
                    //mandamos paquete al fs para que abra el archivo
                    t_paquete *paquete = crear_paquete(TRUNCAR_ARCHIVO);
                    agregar_cadena_a_paquete(paquete, nombre_archivo);
                    //agregar_entero_a_paquete(paquete, tamanio);
                    enviar_paquete(paquete, socket_filesystem);
                    eliminar_paquete(paquete);

                }
                /*
                    fijarnos si existe en la tabla global
                        si existe lo agarramos
                            si se esta escribiendo esperamos a que termine, mandar a cola de lockeados
                            si no se esta escribiendo lockeamos este archivo (cambiar bool o meter en cola)
                    si no existe lo creamos (hace falta esto?)
                        paquete al fs para que lo cree
                        paquete al fs para que lo abra
                */
                break;
        /*
        case POSICIONARSE_ARCHIVO:
            char* nombre_archivo = sacar_cadena_de_paquete(&stream);
            char* posicion = sacar_cadena_de_paquete(&stream);
            eliminar paquete
            break;
        case LEER_ARCHIVO:
            char* nombre_archivo  = sacar_cadena_de_paquete(&stream);
            char* direccion_logica  = sacar_cadena_de_paquete(&stream);
            eliminar paquete
            break;
        case ESCRIBIR_ARCHIVO:
            char* nombre_archivo = sacar_cadena_de_paquete(&stream);
            char* direccion_logica  = sacar_cadena_de_paquete(&stream);
            eliminar paquete
            break; 
        case CERRAR_ARCHIVO:
            char* nombre_archivo  = sacar_cadena_de_paquete(&stream);
            //eliminar paquete
        */
        //termina con cerrar archivokernel
        }
    //}
}

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

bool buscar_en_tabla_de_archivos_abiertos(char* nombre_a_buscar)
{
    printf("\n\nNos fijamos en la tabla\n\n");
    if(archivos_abiertos != NULL && list_size(archivos_abiertos) > 0)
    {
        printf("\n\nTabla no esta vacia buscamos\n\n");
        for (int i = 0; i < list_size(archivos_abiertos); i++)
        {
            //tomamos
            archivo *archivo_de_tabla;
            archivo_de_tabla = (archivo*)list_get(archivos_abiertos,i);

            if (strcmp(nombre_a_buscar,archivo_de_tabla->nombre_archivo) == 0)
            {
                printf("Ya esta abierto: Nombre %s",nombre_a_buscar);
                //devolver a CPU que ya esta abierto (o solo avisar)
                return true;
            }
        }
    } else printf("\n\nTabla esta vacia, osea que no esta abierto\n\n");
    return false;
}


/*
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
		list_add(proceso->archivos_abiertos,archivoActual);
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
		list_add(proceso->archivos_abiertos,archivoActual);
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