#ifndef luxem_rawwrite_h
#define luxem_rawwrite_h

#include "luxem_common.h"

struct luxem_rawwrite_context_t;

typedef luxem_bool_t (*luxem_rawwrite_write_callback_t)(struct luxem_rawwrite_context_t *context, void *user_data, struct luxem_string_t const *string);

struct luxem_rawwrite_context_t *luxem_rawwrite_construct(void);
void luxem_rawwrite_destroy(struct luxem_rawwrite_context_t *context);
void luxem_rawwrite_set_write_callback(struct luxem_rawwrite_context_t *context, luxem_rawwrite_write_callback_t callback, void *user_data);
void luxem_rawwrite_set_pretty(struct luxem_rawwrite_context_t *context, char spacer, size_t multiple);
struct luxem_string_t *luxem_rawwrite_get_error(struct luxem_rawwrite_context_t *context);

luxem_bool_t luxem_rawwrite_object_begin(struct luxem_rawwrite_context_t *context);
luxem_bool_t luxem_rawwrite_object_end(struct luxem_rawwrite_context_t *context);
luxem_bool_t luxem_rawwrite_array_begin(struct luxem_rawwrite_context_t *context);
luxem_bool_t luxem_rawwrite_array_end(struct luxem_rawwrite_context_t *context);
luxem_bool_t luxem_rawwrite_key(struct luxem_rawwrite_context_t *context, struct luxem_string_t const *string);
luxem_bool_t luxem_rawwrite_type(struct luxem_rawwrite_context_t *context, struct luxem_string_t const *string);
luxem_bool_t luxem_rawwrite_primitive(struct luxem_rawwrite_context_t *context, struct luxem_string_t const *string);

#endif

