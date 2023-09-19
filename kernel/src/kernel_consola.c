#include <kernel.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <readline/readline.h>


//kernel.c la llama
//suponemos que con el readline ya te salta la consola intectiva, falta revisar
void inicializar_consola_interactiva() {
  //leer una linea por consola prueba
  char* leer_linea;
  while(1){
    //esto lee la linea
    leer_linea = readline(">");
    // ver como termina la consola, porque capaz con un Ctrl+C y yafu u otra cosa
    
    //aca vamos a ver que pingo puso adentro la linea
    //para saber que hacer (inicializar o finalizar) (falta agregar mas a futuro obviamente)
    consola_parsear_instruccion(leer_linea);
  
  }
  free(leer_linea);
}


void consola_parsear_instruccion(char * leer_linea){

  // INICIAR_PROCESO [PATH] [SIZE] [PRIORIDAD]
  if (string_starts_with(leer_linea, "INICIAR_PROCESO")){
  char* path;
  int tam_proceso_swap;
  int prioridad;
  char** linea_espaciada;
  
  //agarra y divide por espacio, genera una lista de strings
  // ["INICIAR_PROCESO", "[PATH]", "[SIZE]", "[PRIORIDAD]", NULL]
  linea_espaciada = string_n_split(leer_linea, 4, " ");
  
  //guarda en variables
  path = linea_espaciada[1];
  tam_proceso_swap = atoi(linea_espaciada[2]);
  prioridad = atoi(linea_espaciada[3]);

  printf("Proceso iniciado, path %s, size %d, prioridad %d",path, tam_proceso_swap, prioridad);
	
  iniciar_proceso(path, tam_proceso_swap, prioridad);    

  //liberar variables locales (ver si liberamos las demas que declaramos)
  free(linea_espaciada);

  }
  
  printf("busco otra instruccion"); 
  
  //FINALIZAR_PROCESO [PID]
  if (string_starts_with(leer_linea, "FINALIZAR_PROCESO")){
  printf("reconocio finalizar"); 
  t_pcb* pcb;
  char** linea_espaciada;
  
  // ["FINALIZAR_PROCESO" "[PID]" ]
  linea_espaciada = string_n_split(leer_linea, 2, " ");
  
  //guarda en variables
  pcb->pid = linea_espaciada[2];

  printf("Proceso finalizandose con PID %d",pcb->pid);
	
  //proceso_en_exit(pcb);
  
  //liberar variables locales (ver si liberamos las demas que declaramos)
  free(linea_espaciada);

  }

}
/*


Finalizar proceso // FINALIZAR_PROCESO [PID]

*/

void consola_finalizar_proceso(){
  
}

void consola_iniciar_planificacion(){}
void consola_detener_planificacion(){}
void consola_modificar_multiprogramacion(){}
void consola_mostrar_proceso_estado(){}



/*
atrapar fallo de escritura en la consola
*/


/*
 como pingo inicializar una consola interactiva 
*/


//inicializar proceso
//INICIAR_PROCESO(PATH,SIZE,PRIORIDAD);

/*
-Ejecuta un nuevo proceso en base a un archivo dentro del file system de linux
-Creación del proceso (PCB) y dejará el mismo en el estado NEW.
*/

//finalizar proceso
//FINALIZAR_PROCESO(PID);

/*
-Finalizar un proceso que se encuentre dentro del sistema.
-Realiza las mismas operaciones como si el proceso llegara a EXIT por sus caminos habituales
-deberá liberar recursos, archivos y memoria.
*/