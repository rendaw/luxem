#include "luxem_rawread.h"

#include "luxem_internal_common.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#define CONTEXT_ARGS struct luxem_rawread_context_t *context
#define CONTEXT context
#define DATA_ARGS struct luxem_string_t const *data, size_t *eaten
#define DATA data, eaten
#define STATE_ARGS CONTEXT_ARGS, DATA_ARGS
#define STATE CONTEXT, DATA

#define STATE_PROTO(name) \
	static enum result_t name(STATE_ARGS)

#define SET_ERROR(message) \
	context->error.pointer = message; \
	context->error.length = sizeof(message) - 1;

#define ERROR(message) \
	do \
	{ \
	SET_ERROR(message) \
	return result_error; \
	} while(0)

#define PUSH_STATE(state) \
	do \
	{ \
	if (!push_state(context, state)) return result_error; \
	} while(0)

enum result_t
{
	result_continue,
	result_error,
	result_hungry
};

struct stack_t;

typedef enum result_t (*state_signature_t)(STATE_ARGS);

static luxem_bool_t call_void_callback(CONTEXT_ARGS, luxem_rawread_void_callback_t callback);
static luxem_bool_t call_string_callback(CONTEXT_ARGS, luxem_rawread_string_callback_t callback, struct luxem_string_t const *string);
static luxem_bool_t can_eat_one(DATA_ARGS);
static char taste_one(DATA_ARGS);
static char eat_one(DATA_ARGS);
static luxem_bool_t push_state(CONTEXT_ARGS, state_signature_t state);
static void remove_state(CONTEXT_ARGS, struct stack_t *node);
static luxem_bool_t is_whitespace(char const value);
static void eat_whitespace(DATA_ARGS);
STATE_PROTO(state_whitespace);
static enum result_t read_word(STATE_ARGS, struct luxem_string_t *out);
static enum result_t read_words(STATE_ARGS, char delimiter, struct luxem_string_t *out);
STATE_PROTO(state_type);
static enum result_t read_primitive(STATE_ARGS, luxem_bool_t const key);
STATE_PROTO(state_primitive);
STATE_PROTO(state_key);
STATE_PROTO(state_value_phrase);
STATE_PROTO(state_value);
static luxem_bool_t push_object_state(CONTEXT_ARGS);
STATE_PROTO(state_key_separator);
STATE_PROTO(state_object_next);
static luxem_bool_t push_array_state(CONTEXT_ARGS);
STATE_PROTO(state_array_next);

struct stack_t
{
	state_signature_t state;
	struct stack_t *previous;
};

struct luxem_rawread_context_t
{
	struct luxem_string_t error;
	size_t eaten_absolute;
	struct stack_t *state_top;
	struct luxem_rawread_callbacks_t callbacks;
};

luxem_bool_t call_void_callback(CONTEXT_ARGS, luxem_rawread_void_callback_t callback)
{
	if (callback)
		return callback(context, context->callbacks.user_data);
	return luxem_true;
}

luxem_bool_t call_string_callback(CONTEXT_ARGS, luxem_rawread_string_callback_t callback, struct luxem_string_t const *string)
{
	if (callback)
	{
		struct luxem_string_t const *unslashed = unslash(string);
		luxem_bool_t result = callback(context, context->callbacks.user_data, unslashed ? unslashed : string);
		if (unslashed) free((void *)unslashed);
		return result;
	}
	return luxem_true;
}

luxem_bool_t can_eat_one(DATA_ARGS)
{
	return *eaten < data->length;
}

char taste_one(DATA_ARGS)
{
	assert(can_eat_one(DATA));
	return data->pointer[*eaten];
}

char eat_one(DATA_ARGS)
{
	assert(can_eat_one(DATA));
	return data->pointer[(*eaten)++];
}

luxem_bool_t push_state(CONTEXT_ARGS, state_signature_t state)
{
	struct stack_t *new_top = malloc(sizeof(struct stack_t));
	if (!new_top)
	{
		SET_ERROR("Failed to malloc stack element; out of memory?");
		return luxem_false;
	}
	new_top->previous = context->state_top;
	assert(state);
	new_top->state = state;
	context->state_top = new_top;
	return luxem_true;
}

void remove_state(CONTEXT_ARGS, struct stack_t *node)
{
	struct stack_t *above = 0;
	struct stack_t *check = context->state_top;
	assert(node);
	while (check != node)
	{
		if (!check) 
		{
			assert(luxem_false);
			return;
		}
		above = check;
		check = check->previous;
	}
	if (above)
	{
		above->previous = check->previous;
	}
	else
	{
		context->state_top = check->previous;
	}
	free(check);
}

luxem_bool_t is_whitespace(char const value)
{
	switch (value)
	{
		case ' ':
		case '\t':
		case '\n':
			return luxem_true;
		default:
			return luxem_false;
	}
}

void eat_whitespace(DATA_ARGS)
{
	while (luxem_true)
	{
		if (!can_eat_one(DATA)) break;
		if (!is_whitespace(taste_one(DATA))) break;
		eat_one(DATA);
	}
}

STATE_PROTO(state_whitespace)
{
	eat_whitespace(DATA);
	return result_continue;
}

enum result_t read_word(STATE_ARGS, struct luxem_string_t *out)
{
	size_t const start = *eaten;
	luxem_bool_t escaped = luxem_false;
	while (luxem_true)
	{
		if (!can_eat_one(DATA)) break;
		{
			char const next = taste_one(DATA);
			if (escaped)
			{
				eat_one(DATA);
				escaped = luxem_false;
			}
			else if (!is_word_char(next))
			{
				out->pointer = data->pointer + start;
				out->length = *eaten - start;
				return result_continue;
			}
			else eat_one(DATA);
		}
	}
	return result_hungry;
}

enum result_t read_words(STATE_ARGS, char delimiter, struct luxem_string_t *out)
{
	size_t const start = *eaten;
	luxem_bool_t escaped = luxem_false;
	while (luxem_true)
	{
		if (!can_eat_one(DATA)) break;
		{
			char const next = eat_one(DATA);
			if (escaped)
			{
				escaped = luxem_false;
			}
			else if (next == delimiter)
			{
				out->pointer = data->pointer + start;
				out->length = *eaten - 1 - start;
				return result_continue;
			}
		}
	}
	return result_hungry;
}

STATE_PROTO(state_type)
{
	struct luxem_string_t type;
	enum result_t result = read_words(STATE, ')', &type);
	if (result == result_continue)
	{
		luxem_bool_t callback_result = call_string_callback(context, context->callbacks.type, &type);
		if (!callback_result) return result_error;
		return result_continue;
	}

	return result_hungry;
}

enum result_t read_primitive(STATE_ARGS, luxem_bool_t const key)
{
	if (!can_eat_one(DATA)) return result_hungry;

	{
		struct luxem_string_t string;
		enum result_t result;
		if (taste_one(DATA) == '"')
		{
			eat_one(DATA);
			result = read_words(STATE, '"', &string);
		}
		else result = read_word(STATE, &string);
		if (result == result_continue)
		{
			luxem_bool_t const callback_result = 
				key ? call_string_callback(context, context->callbacks.key, &string) :
				call_string_callback(context, context->callbacks.primitive, &string);
			if (!callback_result) return result_error;
			return result_continue;
		}

		return result_hungry;
	}
}

STATE_PROTO(state_primitive)
{
	return read_primitive(STATE, luxem_false);
}

STATE_PROTO(state_key)
{
	return read_primitive(STATE, luxem_true);
}

STATE_PROTO(state_value_phrase)
{
	if (!can_eat_one(DATA)) return result_hungry;
	
	PUSH_STATE(state_value);

	if (taste_one(DATA) == '(')
	{
		eat_one(DATA);
		PUSH_STATE(state_whitespace);
		PUSH_STATE(state_type);
	}

	return result_continue;
}

STATE_PROTO(state_value)
{
	if (!can_eat_one(DATA)) return result_hungry;

	switch (taste_one(DATA))
	{
		case '{':
			eat_one(DATA);
			eat_whitespace(DATA);
			if (!can_eat_one(DATA)) return result_hungry;
			if (taste_one(DATA) == '}') PUSH_STATE(state_object_next);
			else { if (!push_object_state(CONTEXT)) return result_error; }
			call_void_callback(context, context->callbacks.object_begin);
			break;
		case '[': 
			eat_one(DATA);
			eat_whitespace(DATA);
			if (!can_eat_one(DATA)) return result_hungry;
			if (taste_one(DATA) == ']') PUSH_STATE(state_array_next);
			else { if (!push_array_state(CONTEXT)) return result_error; }
			call_void_callback(context, context->callbacks.array_begin);
			break;
		default: 
			PUSH_STATE(state_primitive);
			break;
	}

	return result_continue;
}

luxem_bool_t push_object_state(CONTEXT_ARGS)
{
	PUSH_STATE(state_object_next);
	PUSH_STATE(state_whitespace);
	PUSH_STATE(state_value_phrase);
	PUSH_STATE(state_whitespace);
	PUSH_STATE(state_key_separator);
	PUSH_STATE(state_whitespace);
	PUSH_STATE(state_key);
	return luxem_true;
}

STATE_PROTO(state_key_separator)
{
	if (!can_eat_one(DATA)) return result_hungry;

	if (taste_one(DATA) != ':')
	{
		ERROR("Missing : between key and value.");
	}
	else eat_one(DATA);

	return result_continue;
}

STATE_PROTO(state_object_next)
{
	if (!can_eat_one(DATA)) return result_hungry;

	{
		char next = taste_one(DATA);

		if ((next != ',') && (next != '}'))
		{
			ERROR("Missing , between object elements.");
		}

		if (next == ',')
		{
			eat_one(DATA);
			eat_whitespace(DATA);
			if (!can_eat_one(DATA)) return result_hungry;
			next = taste_one(DATA);
		}

		if (next == '}')
		{
			eat_one(DATA);
			call_void_callback(context, context->callbacks.object_end);
		}
		else { if (!push_object_state(CONTEXT)) return result_error; }

		return result_continue;
	}
}

luxem_bool_t push_array_state(CONTEXT_ARGS)
{
	PUSH_STATE(state_array_next);
	PUSH_STATE(state_whitespace);
	PUSH_STATE(state_value_phrase);
	PUSH_STATE(state_whitespace);
	return luxem_true;
}

STATE_PROTO(state_array_next)
{
	if (!can_eat_one(DATA)) return result_hungry;

	{
		char next = taste_one(DATA);

		if ((next != ',') && (next != ']'))
		{
			ERROR("Missing , between array elements.");
		}

		if (next == ',')
		{
			eat_one(DATA);
			eat_whitespace(DATA);
			if (!can_eat_one(DATA)) return result_hungry;
			next = taste_one(DATA);
		}

		if (next == ']')
		{
			eat_one(DATA);
			call_void_callback(context, context->callbacks.array_end);
		}
		else { if (!push_array_state(CONTEXT)) return result_error; }

		return result_continue;
	}
}

struct luxem_rawread_context_t *luxem_rawread_construct(void)
{
	struct luxem_rawread_context_t *context = malloc(sizeof(struct luxem_rawread_context_t));
	
	if (!context)
	{
		return 0;
	}

	context->error.pointer = 0;
	context->error.length = 0;

	context->eaten_absolute = 0;

	context->state_top = 0;

	if (!push_array_state(context))
	{
		free(context);
		return 0;
	}

	return context;
}

void luxem_rawread_destroy(struct luxem_rawread_context_t *context)
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

struct luxem_rawread_callbacks_t *luxem_rawread_callbacks(CONTEXT_ARGS)
{
	return &context->callbacks;
}

luxem_bool_t luxem_rawread_feed(CONTEXT_ARGS, struct luxem_string_t const *data, size_t *out_eaten)
{
	if (context->error.pointer) return luxem_false;
#ifndef NDEBUG
	context->error.pointer = 0;
	context->error.length = 0;
#endif
	{
		size_t eaten_data = 0;
		size_t *eaten = &eaten_data;
		while (luxem_true)
		{
			struct stack_t *node = context->state_top;

			assert(node);
			if (!node) return luxem_false;

			{
				enum result_t result = node->state(STATE);

				if (result == result_hungry) break;

				if (result == result_error) 
				{
					assert(context->error.pointer);
					if (!context->error.pointer)
					{
						SET_ERROR("Error raised in raw callback but no error message specified.");
					}
					context->eaten_absolute += *eaten - *out_eaten;
					return luxem_false;
				}

				if (result == result_continue)
				{
					remove_state(context, node);
					if (!context->state_top)
					{
						SET_ERROR("Above root depth, exited too many levels during parsing.");
						return luxem_false;
					}
					context->eaten_absolute += *eaten - *out_eaten;
					*out_eaten = *eaten;
				}
			}
		}
	}

	return luxem_true;
}

struct luxem_string_t *luxem_rawread_get_error(CONTEXT_ARGS)
{
	return &context->error;
}

size_t luxem_rawread_get_position(CONTEXT_ARGS)
{
	return context->eaten_absolute;
}

