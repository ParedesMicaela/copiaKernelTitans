#ifndef DICTIONARY_INT_H_
#define DICTIONARY_INT_H_

#include <commons/collections/dictionary.h>
#include <commons/collections/list.h>
#include <stdbool.h>
#include <stdlib.h>
#include <netdb.h>
#include <stdio.h>

typedef struct
{
	t_dictionary* dictionary;
} t_dictionary_int;

extern t_dictionary_int* dictionary_int_create();
extern void dictionary_int_put(t_dictionary_int* diccionario, uint32_t key, void *data);
extern void *dictionary_int_get(t_dictionary_int* diccionario, uint32_t key);
extern void *dictionary_int_remove(t_dictionary_int* diccionario, uint32_t key);
extern void dictionary_int_remove_and_destroy(t_dictionary_int* diccionario, uint32_t key, void(*data_destroyer)(void*));
extern void dictionary_int_iterator(t_dictionary_int* diccionario, void(*closure)(uint32_t,void*));
extern void dictionary_int_clean(t_dictionary_int* diccionario);
extern void dictionary_int_clean_and_destroy_elements(t_dictionary_int* diccionario, void(*data_destroyer)(void*));
extern bool dictionary_intt_has_key(t_dictionary_int* diccionario, uint32_t key);
extern bool dictionary_int_is_empty(t_dictionary_int* diccionario);
extern int dictionary_int_size(t_dictionary_int* diccionario);
extern void dictionary_int_destroy(t_dictionary_int* diccionario);
extern void dictionary_int_destroy_and_destroy_elements(t_dictionary_int* diccionario, void(*data_destroyer)(void*));
extern t_list* dictionary_int_toList(t_dictionary_int* diccionario);

#endif /*DICTIONARY_INT_H_*/
