#ifndef luxem_common_h
#define luxem_common_h

#include <string.h>

#define luxem_bool_t int
#define luxem_true 1
#define luxem_false 0

struct luxem_string_t
{
	char const *pointer;
	size_t length;
};

#define luxem_print_string(string, stream) \
do \
{ \
	size_t i; \
	size_t remaining = string->length; \
	while ((remaining > 0) && \
		((i = fwrite(string->pointer, 1, remaining, stream)) > 0)) \
		remaining -= i; \
} while (0)

#endif

