#ifndef luxem_internal_common_h
#define luxem_internal_common_h

#include "luxem_common.h"

luxem_bool_t is_word_char(char value);
luxem_bool_t is_word(struct luxem_string_t const *string);

struct luxem_string_t const *slash(struct luxem_string_t const *string, char const delimiter);
struct luxem_string_t const *unslash(struct luxem_string_t const *string);

#define LUXEM_BUFFER_BLOCK_SIZE 8

#endif

