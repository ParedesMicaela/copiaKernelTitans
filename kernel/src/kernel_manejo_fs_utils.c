#include "kernel.h"

bool es_una_operacion_con_archivos(char* motivo_bloqueo) {
        string_equals_ignore_case(motivo_bloqueo, "F_WRITE") 
        || string_equals_ignore_case(motivo_bloqueo, "F_TRUNCATE")
        || string_equals_ignore_case(motivo_bloqueo, "F_READ");
}

/*
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

        //el proceso se bloquea hasta que el fs me informe la finalizacion de la operacion
        int respuesta = 0;
        recv(socket_filesystem, &respuesta, sizeof(int),0);

        if (respuesta != 1)
        {
            log_error(kernel_logger, "Hubo un error con la respuesta de fs\n");
        }else{
            proceso_en_execute(proceso);
        }
        break;

*/