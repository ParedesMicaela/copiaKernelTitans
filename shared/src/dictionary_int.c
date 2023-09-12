#include "dictionary_int.h"

t_dictionary_int* dictionary_int_create()
{
	t_dictionary_int* nuevo_diccionario_int = malloc(sizeof(t_dictionary_int));
	nuevo_diccionario_int->dictionary = dictionary_create();
	return nuevo_diccionario_int;
}

void dictionary_int_put(t_dictionary_int* diccionario, uint32_t key, void *data)
{
	char keyArray[10];
	sprintf(keyArray, "%d", key);
	dictionary_put(diccionario->dictionary, keyArray, data);
}

void *dictionary_int_get(t_dictionary_int* diccionario, uint32_t key)
{
	char keyArray[10];
	sprintf(keyArray, "%d", key);
	return dictionary_get(diccionario->dictionary, keyArray);
}

void *dictionary_int_remove(t_dictionary_int* diccionario, uint32_t key)
{
	char keyArray[10];
		sprintf(keyArray, "%d", key);
	return dictionary_remove(diccionario->dictionary, keyArray);
}

void dictionary_int_remove_and_destroy(t_dictionary_int* diccionario, uint32_t key, void(*data_destroyer)(void*))
{
	char keyArray[10];
	sprintf(keyArray, "%d", key);
	dictionary_remove_and_destroy(diccionario->dictionary, keyArray, data_destroyer);
}

void dictionary_int_iterator(t_dictionary_int* diccionario, void(*closure)(uint32_t,void*))
{
	int table_index;
	for (table_index = 0; table_index < diccionario->dictionary->table_max_size; table_index++) {
		t_hash_element *element = diccionario->dictionary->elements[table_index];

		while (element != NULL) {
			uint32_t key;
			sscanf(element->key, "%d", &key);
			closure(key, element->data);
			element = element->next;
		}
	}
}

void dictionary_int_clean(t_dictionary_int* diccionario)
{
	dictionary_clean(diccionario->dictionary);
}

void dictionary_int_clean_and_destroy_elements(t_dictionary_int* diccionario, void(*data_destroyer)(void*))
{
	dictionary_clean_and_destroy_elements(diccionario->dictionary, data_destroyer);
}

bool dictionary_int_has_key(t_dictionary_int* diccionario, uint32_t key)
{
	char keyArray[10];
	sprintf(keyArray, "%d", key);
	return dictionary_has_key(diccionario->dictionary, keyArray);
}

bool dictionary_int_is_empty(t_dictionary_int* diccionario)
{
	return dictionary_is_empty(diccionario->dictionary);
}

int dictionary_int_size(t_dictionary_int* diccionario)
{
	return dictionary_size(diccionario->dictionary);
}

void dictionary_int_destroy(t_dictionary_int* diccionario)
{
	dictionary_destroy(diccionario->dictionary);
	free(diccionario);
}

void dictionary_int_destroy_and_destroy_elements(t_dictionary_int* diccionario, void(*data_destroyer)(void*))
{
	dictionary_destroy_and_destroy_elements(diccionario->dictionary, data_destroyer);
}


t_list* dictionary_int_toList(t_dictionary_int* diccionario)
{
	t_list* lista = list_create();

	int table_index;
	for (table_index = 0; table_index < diccionario->dictionary->table_max_size; table_index++) {
		t_hash_element *element = diccionario->dictionary->elements[table_index];

		while (element != NULL) {
			list_add(lista, element->data);
			element = element->next;
		}
	}

	return lista;
}
