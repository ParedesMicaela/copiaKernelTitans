/*
/// COMUNICACIÓN A MEMORIA ///
void* conexion_inicial_memoria(){

	int codigo_memoria;

	while(1){
		codigo_memoria=recibir_operacion(socket_memoria);
		switch(codigo_memoria){
			case PAQUETE:
				log_info(filesystem_logger,"Recibi configuracion por handshake \n");
				return NULL;
			break;
			case -1:
				log_error(filesystem_logger, "Fallo la comunicacion. Abortando \n");
				return (void*)(EXIT_FAILURE);
			break;
			default:
				 log_warning(filesystem_logger, "Operacion desconocida \n");
			break;
		}
	}
	 return (void*)(EXIT_SUCCESS);
}
*/
