#include "kernel.h"

//aca implementamos la tabla de archivos globales
typedef struct tabla_archivos {
    FILE *ARCHIVO;      
    char *nombre_archivo;
    char tipo_apertura;
    int tamanio;
    //recursos asignados??
    //algo mas??
    //ponerlo en kernel.h cuando termines
} tabla_archivos;

tabla_archivos *tabla_archivos_abiertos = NULL; // Lista enlazada de archivos abiertos

//cola de locks (peticiones)
t_list cola_locks_escritura; 
t_list cola_locks_lectura;
t_list cola_locks_bloqueados; // aca guardamos los que se bloquean, esperando a que se resuelva el que se esta ejecutando

void atender_peticiones_al_fs()
{
    printf("\nEsperamos recibir paquete\n");
    
    while(1)
    {
        log_info(kernel_logger, "Espero recibir paquete");
        
        //recibimos eltipo de isntrucicon y el paquete con todo adentro
        t_paquete *paquete = recibir_paquete(socket_cpu_dispatch);
        void *stream = paquete->buffer->stream;
        log_info(kernel_logger, "Ya recibi paquete, ahora hariamos el switch");

        //el paquete tiene que tener ademas el nombre del archivo y el modo de apertura

        /*
        switch(archivo)
            case fopen abrirArchivoKernel
            case frwrite
            case fseek
        termina con cerrar archivokernel
        */
    }
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