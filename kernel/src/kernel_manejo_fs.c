#include "kernel.h"

//aca implementamos la tabla de archivos globales
typedef struct archivo {
    FILE *ARCHIVO;          //direccion de memoria del archivo      
    char *nombre_archivo;   //nombre (identificador)
    bool lockeado;          //para ver si esta lockeado o no (al pedo, porque creo que lo ponemos en una cola y listo)
    //char tipo_apertura;   //esto puede ser que no sea necesario, pero como es de archivos abiertos capaz si
    //int tamanio;          //tamanio en memoria... necesario?? 
    //recursos asignados??
    //algo mas??
    //ponerlo en kernel.h cuando termines
    archivo *siguiente;
} archivo;

//tabla_archivos *lista_archivos = malloc(sizeof(tabla_archivos)); es dinamica???
archivo tabla_archivos_abiertos;
t_list tabla_archivos_abiertos;


/*
//en kernel.h
//cola de locks (peticiones)
t_list cola_locks_escritura; 
t_list cola_locks_lectura;
t_list cola_locks_bloqueados; // aca guardamos los que se bloquean, esperando a que se resuelva el que se esta ejecutando
*/

archivo *archivo_abierto;
bool existe_en_tabla = false;

void manejo_de_f_open(t_pcb* proceso) {

//abrirArchivoKernel();

printf("\n\nKernel recibio paquete de CPU para abrir archivo\n\n");
                
for (int i = 0; i < list_size(tabla_archivos_abiertos); i++)
    {
        archivo_abierto = (archivo*)list_get(tabla_archivos_abiertos, i);
            if (strcmp(nombre_archivo, archivo_abierto->nombre_archivo) == 0)
                {
                        existe_en_tabla = true;
                            // ocurre algo
                        }
                        else
                        {
                            existe_en_tabla = false;
                        }
                    }
                /*  fijarnos si existe en la tabla global
                        si existe mandamos paquete a fs para que le haga F_OPEN
                            esperamos OK
                        si no existe mandamos paquete al fs para que lo cree
                            esperamos que devuelva OK
                            agregamos a la tabla
                */  

                //crearArchivoKernel();
                printf("\n\nKernel recibio paquete de CPU para crear archivo\n\n");
                /*
                    fijarnos si existe en la tabla global
                        si no existe, paquete al fs para que lo cree
                            esperamos a que devuelva OK
                        si existe, paquete al fs para que lo abra
                            esperamos a que devuelva OK
                */
}
        
void manejo_de_f_create(t_pcb* proceso) 
    
        {
          
 
            {
                
                break;
            }
            case CREAR_ARCHIVO:
	        {
               
                break;
            }
            case TRUNCAR_ARCHIVO:
            {
                //truncarArchivoKernel();
                printf("\n\nKernel recibio paquete de CPU para truncar archivo\n\n");
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
            }
            /*
            case LEER_ARCHIVO:
            {
                //leerArchivoKernel();
                break;
            }
            case ESCRIBIR_ARCHIVO:
            {
                //escribirArchivoKernel();
                break; 
            }
            */
            //termina con cerrar archivokernel
        }
        
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