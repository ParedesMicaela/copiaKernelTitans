// Biblioteca usada para usar instrucciones y registros cpu
#ifndef INSTRUCCIONES_H_
#define INSTRUCCIONES_H_

#include "operaciones.h"

// Enums y Estructuras

struct instruccion {
    
    codigo_instrucciones tipoInstruccion;
    uint32_t operando1;
    uint32_t operando2;
    uint32_t operando3;
    t_registros_cpu registro1;
    t_registros_cpu registro2;
    char *valorSet;
    char *dispositivoIo;
    char *nombreArchivo;
    char *toString;

};
typedef struct instruccion t_instruccion;

struct info_instruccion {

    uint32_t operando1;
    uint32_t operando2;
    uint32_t operando3;
    t_registros_cpu registro1;
    t_registros_cpu registro2;
    char *valorSet;
    char *dispositivoIo;
    char *nombreArchivo;
    
};
typedef struct info_instruccion t_info_instruccion;

#endif 