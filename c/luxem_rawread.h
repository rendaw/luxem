#ifndef luxem_rawread_h
#define luxem_rawread_h

#include "luxem_common.h"

#include <stdio.h>

struct luxem_rawread_callbacks_t;
struct luxem_rawread_context_t;

typedef luxem_bool_t (*luxem_rawread_void_callback_t)(struct luxem_rawread_context_t *context, void *user_data);
typedef luxem_bool_t (*luxem_rawread_string_callback_t)(struct luxem_rawread_context_t *context, void *user_data, struct luxem_string_t const *string);

struct luxem_rawread_callbacks_t
{
	luxem_rawread_void_callback_t 
		object_begin, 
		object_end, 
		array_begin, 
		array_end;
	luxem_rawread_string_callback_t 
		key, 
		type, 
		primitive;
	void *user_data;
};

struct luxem_rawread_context_t *luxem_rawread_construct(void);
void luxem_rawread_destroy(struct luxem_rawread_context_t *context);
struct luxem_rawread_callbacks_t *luxem_rawread_callbacks(struct luxem_rawread_context_t *context);
luxem_bool_t luxem_rawread_feed(struct luxem_rawread_context_t *context, struct luxem_string_t const *data, size_t *out_eaten, luxem_bool_t finish);
luxem_bool_t luxem_rawread_feed_file(struct luxem_rawread_context_t *context, FILE *file, luxem_rawread_void_callback_t block_callback, luxem_rawread_void_callback_t unblock_callback);
struct luxem_string_t *luxem_rawread_get_error(struct luxem_rawread_context_t *context);
size_t luxem_rawread_get_position(struct luxem_rawread_context_t *context);

struct luxem_string_t const *luxem_from_ascii16(struct luxem_string_t const *data, struct luxem_string_t *error);

#endif

