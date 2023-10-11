#include "kernel.h"
#include "stdlib.h"
#include "stdio.h"
#include "unistd.h"
#include "readline/readline.h"
#include "operaciones.h"

void inicializar_consola_interactiva() {
  char* leer_linea;
  while(1) {
    leer_linea = readline(">");  // Lee una línea de la consola
    if (leer_linea) {
      consola_parsear_instruccion(leer_linea);  // Llama a la función para parsear la instrucción
      free(leer_linea);  // Libera la memoria asignada para la línea
    } else {
      // Maneja la falla de lectura de la línea o sale del bucle en ciertas condiciones
      break;
    }
  }
}

void parse_iniciar_proceso(char *line) {
  char path[256];  // Suponiendo una longitud máxima de ruta de 255 caracteres
  int tam_proceso_swap;
  int prioridad;
  char **linea_espaciada = string_n_split(line, 4, " ");  // Divide la línea en tokens
  
  if (linea_espaciada && linea_espaciada[1] && linea_espaciada[2] && linea_espaciada[3]) {
    if (sscanf(linea_espaciada[1], "\"%255[^\"]\"", path) == 1 &&
        sscanf(linea_espaciada[2], "%d", &tam_proceso_swap) == 1 &&
        sscanf(linea_espaciada[3], "%d", &prioridad) == 1) {
      // Extrae la ruta, el tamaño y la prioridad, y los asigna a las variables
      printf("Proceso iniciandose, path %s, size %d, prioridad %d\n", path, tam_proceso_swap, prioridad);
      iniciar_proceso(path, tam_proceso_swap, prioridad);
      printf("Proceso iniciado\n");
    }
    free(linea_espaciada);  // Libera la memoria asignada para los tokens
  }
}

void parse_finalizar_proceso(char *line) {
  char **linea_espaciada = string_n_split(line, 2, " ");  // Divide la línea en tokens
  
  if (linea_espaciada && linea_espaciada[1]) {
    int pid;
    if (sscanf(linea_espaciada[1], "%d", &pid) == 1) {
      // Extrae el PID y lo asigna a la variable
      printf("Intentamos entrar a finalizar proceso\n");
      consola_finalizar_proceso(pid);
    }
    free(linea_espaciada);  // Libera la memoria asignada para los tokens
  }
}

void consola_parsear_instruccion(char *leer_linea) {
  if (string_contains(leer_linea, "INICIAR_PROCESO")) {
    parse_iniciar_proceso(leer_linea);  // Llama a la función para parsear la instrucción de inicio de proceso
  } else if (string_contains(leer_linea, "FINALIZAR_PROCESO")) {
    parse_finalizar_proceso(leer_linea);  // Llama a la función para parsear la instrucción de finalización de proceso
  } else {
    printf("Comando desconocido: %s\n", leer_linea);  // Imprime un mensaje de error cuando se encuentra un comando desconocido
  }
  printf("Busco otra instrucción\n");
}





void consola_iniciar_planificacion(){}
void consola_detener_planificacion(){}
void consola_modificar_multiprogramacion(){}
void consola_mostrar_proceso_estado(){}



/*
atrapar fallo de escritura en la consola
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