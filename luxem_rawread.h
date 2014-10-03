#ifndef luxem_rawread_h
#define luxem_rawread_h

#include "luxem_common.h"

enum luxem_rawread_conclusion_t
{
	luxem_rawread_cont,
	luxem_rawread_error,
	luxem_rawread_hungry
};

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
struct luxem_rawread_callbacks_t *luxem_rawread_callbacks(struct luxem_rawread_context_t *context);
void luxem_rawread_destroy(struct luxem_rawread_context_t *context);
luxem_bool_t luxem_rawread_feed(struct luxem_rawread_context_t *context, struct luxem_string_t const *data, size_t *out_eaten);
struct luxem_string_t const *luxem_rawread_get_error(struct luxem_rawread_context_t *context);
size_t luxem_rawread_get_position(struct luxem_rawread_context_t *context);

#endif

