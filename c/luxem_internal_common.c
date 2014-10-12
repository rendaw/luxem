#include "luxem_internal_common.h"

#include <stdlib.h>
#include <assert.h>

#include <stdio.h> /* debug */

luxem_bool_t is_word_char(char value)
{
	switch (value)
	{
		case ' ':
		case '\t':
		case '\n':
		case ':':
		case ',':
		case '(':
		case ')':
		case '{':
		case '}':
		case '[':
		case ']':
		case '"':
			return luxem_false;
		default: return luxem_true;
	}
}

luxem_bool_t is_word(struct luxem_string_t const *string)
{
	int index;
	for (index = 0; index < string->length; ++index)
		if (!is_word_char(string->pointer[index])) return luxem_false;
	return luxem_true;
}

static struct luxem_string_t *malloc_string(size_t length)
{
	struct luxem_string_t *out = malloc(sizeof(struct luxem_string_t) + length);
	out->pointer = (char *)out + sizeof(struct luxem_string_t);
	out->length = length;
	return out;
}

struct luxem_string_t const *slash(struct luxem_string_t const *string, char const delimiter)
{
	size_t quotes_and_slashes = 0;
	{
		int index;
		for (index = 0; index < string->length; ++index)
		{
			char const read = string->pointer[index];
			if ((read == '\\') || (read == delimiter))
				++quotes_and_slashes;
		}
	}
	if (quotes_and_slashes == 0) return 0;
	else
	{
		struct luxem_string_t *replacement = malloc_string(string->length + quotes_and_slashes);
		if (!replacement) return 0; /* TODO Set the error somehow as well */
		else
		{
			char *cursor = (char *)replacement->pointer; /* We control this memory */
			int index;
			for (index = 0; index < string->length; ++index)
			{
				char const read = string->pointer[index];
				if ((read == '\\') || (read == delimiter))
					*(cursor++) = '\\';
				*(cursor++) = read;
			}
			assert(cursor == replacement->pointer + replacement->length);
			return replacement;
		}
	}
}

struct luxem_string_t const *unslash(struct luxem_string_t const *string)
{
	size_t unescaped_slashes = 0;
	{
		int index;
		luxem_bool_t escaped = luxem_false;
		for (index = 0; index < string->length; ++index)
		{
			if (escaped) escaped = luxem_false;
			else if (string->pointer[index] == '\\')
			{
				escaped = luxem_true;
				++unescaped_slashes;
			}
		}
	}
	if (unescaped_slashes == 0) return 0;
	{
		struct luxem_string_t *replacement = malloc_string(string->length - unescaped_slashes);
		if (!replacement) return 0;
		else
		{
			char *cursor = (char *)replacement->pointer; /* We control this memory */
			int index;
			luxem_bool_t escaped = luxem_false;
			for (index = 0; index < string->length; ++index)
			{
				char const read = string->pointer[index];
				if (escaped) escaped = luxem_false;
				else if (read == '\\')
				{
					escaped = luxem_true;
					continue;
				}
				*(cursor++) = read;
			}
			assert(cursor == replacement->pointer + replacement->length);
			return replacement;
		}
	}
}

luxem_bool_t luxem_buffer_construct(struct luxem_buffer_t *buffer)
{
	buffer->allocated = LUXEM_BUFFER_BLOCK_SIZE * 2;
	buffer->pointer = malloc(buffer->allocated);
	return buffer->pointer == 0 ? luxem_false : luxem_true;
}

luxem_bool_t luxem_buffer_resize(struct luxem_buffer_t *buffer, size_t trim_front, size_t expand)
{
	assert(trim_front <= buffer->allocated);
	if (expand < trim_front)
	{
		memmove(buffer->pointer, buffer->pointer + trim_front, buffer->allocated - trim_front);
	}
	else
	{
		size_t new_length = buffer->allocated * 2;
		char *out = malloc(new_length);
		if (!out) 
		{
			free(buffer->pointer);
			return luxem_false;
		}
		memcpy(out, buffer->pointer + trim_front, buffer->allocated - trim_front);
		free(buffer->pointer);
		buffer->pointer = out;
		buffer->allocated = new_length;
	}
	return luxem_true;
}

void luxem_buffer_destroy(struct luxem_buffer_t *buffer)
{
	free(buffer->pointer);
}

