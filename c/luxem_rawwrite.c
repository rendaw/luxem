#include "luxem_rawwrite.h"

#include "luxem_internal_common.h"

#include <stdlib.h>
#include <assert.h>
#include <alloca.h>

#include <stdio.h> /* debug */

#define SET_ERROR_TARGET(message, target) \
	do { \
	target->pointer = message; \
	target->length = sizeof(message) - 1; \
	} while(0)

#define ERROR(message) \
	do { \
	context->error.pointer = message; \
	context->error.length = sizeof(message) - 1; \
	return luxem_false; \
	} while(0)

#define CHECK() \
	do { \
		if (context->error.pointer) return luxem_false; \
		assert(context->state_top); \
	} while (0)

#define CHECK_STATE(states) \
	do { \
	if (!(context->state_top->state & (states))) \
		ERROR("Invalid state; state must be " #states "."); \
	} while (0)

#define IS_STATE(states) \
	(context->state_top->state & (states))

#define PUSH_STATE(state) \
	do { \
	if (!push_state(context, state)) return luxem_false; \
	} while (0)

#define POP_STATE(state) \
	do { \
	if (!pop_state(context)) return luxem_false; \
	} while (0)

#define WRITE(string) \
	do { \
		if (!context->write_callback) ERROR("You must set write_callback before writing."); \
		if (!context->write_callback(context, context->user_data, string)) return luxem_false; \
	} while (0)

#define WRITE_STRING(message) \
	do { \
	char const error_temp[] = message; \
	struct luxem_string_t string; \
	string.pointer = error_temp; \
	string.length = sizeof(error_temp) - 1; \
	WRITE(&string); \
	} while(0)

#define INDENT() \
	do { \
	if (context->pretty) \
	{ \
		size_t const count = context->pretty_indent_multiple * context->indentation; \
		if (count < 128) \
		{ \
			struct luxem_string_t buffer; \
			char *pointer = alloca(count); \
			buffer.length = count; \
			buffer.pointer = pointer; \
			memset(pointer, context->pretty_indenter, count); \
			WRITE(&buffer); \
		} \
		else \
		{ \
			struct luxem_string_t buffer; \
			int i; \
			buffer.length = 1; \
			buffer.pointer = &context->pretty_indenter; \
			for (i = 0; i < count; ++i) \
				WRITE(&buffer); \
		} \
	} \
	} while(0)

struct stack_t;
struct luxem_rawwrite_context_t;

enum luxem_state_t
{
	state_object = 1 << 0,
	state_array = 1 << 1,
	state_value_phrase = 1 << 2,
	state_value = 1 << 3
};

static luxem_bool_t push_state(struct luxem_rawwrite_context_t *context, enum luxem_state_t state);
static luxem_bool_t pop_state(struct luxem_rawwrite_context_t *context);

struct stack_t
{
	enum luxem_state_t state;
	struct stack_t *previous;
};

struct luxem_rawwrite_context_t
{
	struct luxem_string_t error;
	struct stack_t *state_top;
	luxem_rawwrite_write_callback_t write_callback;
	void *user_data;
	luxem_bool_t pretty;
	char pretty_indenter;
	size_t pretty_indent_multiple;
	size_t indentation;
};

static luxem_bool_t push_state(struct luxem_rawwrite_context_t *context, enum luxem_state_t state)
{
	struct stack_t *new_top = malloc(sizeof(struct stack_t));
	if (!new_top) ERROR("Failed to malloc stack element; out of memory?");
	new_top->previous = context->state_top;
	assert(state);
	context->state_top = new_top;
	new_top->state = state;
	switch (state)
	{
		case state_object:
		case state_array:
			++context->indentation;
			break;
		default: break;
	}
	return luxem_true;
}

static luxem_bool_t pop_state(struct luxem_rawwrite_context_t *context)
{
	struct stack_t *check = context->state_top;
	{
		struct stack_t *top = context->state_top;
		while (top)
			top = top->previous;
	}
	assert(context->state_top); /* Should be checked in CHECK() */
	switch (context->state_top->state)
	{
		case state_object:
		case state_array:
			assert(context->indentation > 1);
			--context->indentation;
			break;
		default: break;
	}
	context->state_top = check->previous;
	free(check);
	if (!context->state_top) ERROR("Invalid state; empty stack.  Did you close too many objects or arrays?");
	return luxem_true;
}

struct luxem_rawwrite_context_t *luxem_rawwrite_construct(void)
{
	struct luxem_rawwrite_context_t *context = malloc(sizeof(struct luxem_rawwrite_context_t));
	
	if (!context)
	{
		return 0;
	}

	context->error.pointer = 0;
	context->error.length = 0;

	context->state_top = 0;

	context->write_callback = 0;
	
	context->pretty = luxem_false;
	context->pretty_indenter = '\t';
	context->pretty_indent_multiple = 1;

	context->indentation = 0;
	PUSH_STATE(state_array);
	context->indentation = 0;

	return context;
}

void luxem_rawwrite_destroy(struct luxem_rawwrite_context_t *context)
{
	struct stack_t *top = 0;
	struct stack_t *node = context->state_top;
	while (node)
	{
		top = node;
		node = node->previous;
		free(top);
	}

	free(context);
}

struct luxem_string_t *luxem_rawwrite_get_error(struct luxem_rawwrite_context_t *context)
{
	return &context->error;
}

void luxem_rawwrite_set_write_callback(struct luxem_rawwrite_context_t *context, luxem_rawwrite_write_callback_t callback, void *user_data)
{
	context->write_callback = callback;
	context->user_data = user_data;
}

void luxem_rawwrite_set_pretty(struct luxem_rawwrite_context_t *context, char indenter, size_t multiple)
{
	context->pretty = luxem_true;
	context->pretty_indenter = indenter;
	context->pretty_indent_multiple = multiple;
}

luxem_bool_t luxem_rawwrite_object_begin(struct luxem_rawwrite_context_t *context)
{
	CHECK();
	CHECK_STATE(state_value_phrase | state_value | state_array);
	if (IS_STATE(state_value_phrase | state_value)) POP_STATE();
	else INDENT();
	WRITE_STRING("{");
	if (context->pretty) WRITE_STRING("\n");
	PUSH_STATE(state_object);
	return luxem_true;
}

luxem_bool_t luxem_rawwrite_object_end(struct luxem_rawwrite_context_t *context)
{
	CHECK();
	CHECK_STATE(state_object);
	POP_STATE();
	INDENT();
	WRITE_STRING("},");
	if (context->pretty) WRITE_STRING("\n");
	return luxem_true;
}

luxem_bool_t luxem_rawwrite_array_begin(struct luxem_rawwrite_context_t *context)
{
	CHECK();
	CHECK_STATE(state_value_phrase | state_value | state_array);
	if (IS_STATE(state_value_phrase | state_value)) POP_STATE();
	else INDENT();
	WRITE_STRING("[");
	if (context->pretty) WRITE_STRING("\n");
	PUSH_STATE(state_array);
	return luxem_true;
}

luxem_bool_t luxem_rawwrite_array_end(struct luxem_rawwrite_context_t *context)
{
	CHECK();
	CHECK_STATE(state_array);
	POP_STATE();
	INDENT();
	WRITE_STRING("],");
	if (context->pretty) WRITE_STRING("\n");
	return luxem_true;
}

luxem_bool_t luxem_rawwrite_key(struct luxem_rawwrite_context_t *context, struct luxem_string_t const *string)
{
	CHECK();
	CHECK_STATE(state_object);
	INDENT();
	{
		struct luxem_string_t const *slashed = slash(string, '"');
		if (is_word(string)) WRITE(slashed ? slashed : string);
		else 
		{
			WRITE_STRING("\"");
			WRITE(slashed ? slashed : string);
			WRITE_STRING("\"");
		}
		if (slashed) free((void *)slashed);
	}
	WRITE_STRING(":");
	if (context->pretty) WRITE_STRING(" ");
	PUSH_STATE(state_value_phrase);
	return luxem_true;
}

luxem_bool_t luxem_rawwrite_type(struct luxem_rawwrite_context_t *context, struct luxem_string_t const *string)
{
	CHECK();
	CHECK_STATE(state_value_phrase | state_array);
	if (IS_STATE(state_value_phrase)) POP_STATE();
	else INDENT();
	{
		struct luxem_string_t const *slashed = slash(string, ')');
		WRITE_STRING("(");
		WRITE(slashed ? slashed : string);
		WRITE_STRING(")");
		if (slashed) free((void *)slashed);
	}
	if (context->pretty) WRITE_STRING(" ");
	PUSH_STATE(state_value);
	return luxem_true;
}

luxem_bool_t luxem_rawwrite_primitive(struct luxem_rawwrite_context_t *context, struct luxem_string_t const *string)
{
	CHECK();
	CHECK_STATE(state_value_phrase | state_value | state_array);
	if (IS_STATE(state_value_phrase | state_value)) POP_STATE();
	else INDENT();
	{
		struct luxem_string_t const *slashed = slash(string, '"');
		if (is_word(string)) WRITE(string);
		else 
		{
			WRITE_STRING("\"");
			WRITE(slashed ? slashed : string);
			WRITE_STRING("\"");
		}
		if (slashed) free((void *)slashed);
	}
	WRITE_STRING(",");
	if (context->pretty) WRITE_STRING("\n");
	return luxem_true;
}

struct luxem_string_t const *luxem_to_ascii16(struct luxem_string_t const *data, struct luxem_string_t *error)
{
	assert(data);
	{
		size_t const new_length = data->length << 1;
		struct luxem_string_t *out = malloc(sizeof(struct luxem_string_t) + new_length);
		if (!out) 
		{
			SET_ERROR_TARGET("Failed to allocate output memory", error);
			return 0;
		}
		{
			int index;
			char *cursor = (char *)out + sizeof(struct luxem_string_t);
			out->pointer = cursor;
			out->length = new_length;
			for (index = 0; index < data->length; ++index)
			{
				assert((unsigned char)(data->pointer[index] >> 4) < 16);
				assert((unsigned char)(data->pointer[index] & 0x0F) < 16);
				*(cursor++) = (unsigned char)'a' + ((unsigned char)data->pointer[index] >> 4);
				*(cursor++) = (unsigned char)'a' + ((unsigned char)data->pointer[index] & 0x0F);
			}
			assert(cursor == out->pointer + new_length);
		}
		return out;
	}
}

