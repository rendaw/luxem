#ifndef luxem_internal_common_h
#define luxem_internal_common_h

#include "luxem_common.h"

luxem_bool_t is_word_char(char value);
luxem_bool_t is_word(struct luxem_string_t const *string);

struct luxem_string_t const *slash(struct luxem_string_t const *string, char const delimiter);
struct luxem_string_t const *unslash(struct luxem_string_t const *string);

struct luxem_buffer_t
{
	size_t allocated;
	char *pointer;
};

#define LUXEM_BUFFER_BLOCK_SIZE 8

luxem_bool_t luxem_buffer_construct(struct luxem_buffer_t *buffer);
luxem_bool_t luxem_buffer_resize(struct luxem_buffer_t *buffer, size_t trim_front, size_t expand);
void luxem_buffer_destroy(struct luxem_buffer_t *buffer);

#endif

