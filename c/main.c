#include "luxem_rawread.h"

#include <stdio.h>
#include <assert.h>

size_t depth = 0;

luxem_bool_t object_begin_callback(struct luxem_rawread_context_t *context, void *user_data)
{
	printf("%03lu Beginning object\n", depth);
	++depth;
	return luxem_true;
}

luxem_bool_t object_end_callback(struct luxem_rawread_context_t *context, void *user_data)
{
	--depth;
	printf("%03lu Ending object\n", depth);
	return luxem_true;
}

luxem_bool_t array_begin_callback(struct luxem_rawread_context_t *context, void *user_data)
{
	printf("%03lu Beginning array\n", depth);
	++depth;
	return luxem_true;
}

luxem_bool_t array_end_callback(struct luxem_rawread_context_t *context, void *user_data)
{
	--depth;
	printf("%03lu Ending array\n", depth);
	return luxem_true;
}

luxem_bool_t key_callback(struct luxem_rawread_context_t *context, void *user_data, struct luxem_string_t const *string)
{
	printf("%03lu Key: ", depth);
	luxem_print_string(string, stdout);
	printf("\n");
	return luxem_true;
}

luxem_bool_t type_callback(struct luxem_rawread_context_t *context, void *user_data, struct luxem_string_t const *string)
{
	printf("%03lu Type: ", depth);
	luxem_print_string(string, stdout);
	printf("\n");
	return luxem_true;
}

luxem_bool_t primitive_callback(struct luxem_rawread_context_t *context, void *user_data, struct luxem_string_t const *string)
{
	printf("%03lu Primitive: ", depth);
	luxem_print_string(string, stdout);
	printf("\n");
	return luxem_true;
}

int main(int argc, char **argv)
{
	struct luxem_rawread_context_t *reader = luxem_rawread_construct();
	luxem_rawread_callbacks(reader)->object_begin = object_begin_callback;
	luxem_rawread_callbacks(reader)->object_end = object_end_callback;
	luxem_rawread_callbacks(reader)->array_begin = array_begin_callback;
	luxem_rawread_callbacks(reader)->array_end = array_end_callback;
	luxem_rawread_callbacks(reader)->key = key_callback;
	luxem_rawread_callbacks(reader)->type = type_callback;
	luxem_rawread_callbacks(reader)->primitive = primitive_callback;

	{
		struct luxem_string_t input;
		char const data[] = 
			"\n"
			"{\n"
			"	key1: val1,key1.5:val1.5,\n"
			"	\"key2 with spaces\": \"val2 with spaces\",\n"
			"	key3: (type3) val3,\n"
			"	key4:(type4)\"val4 with spaces\",\n"
			"	key5: [\n"
			"		val5.1,\n"
			"		\"val5.2\",\n"
			"		(type5.3) {\n"
			"			val5.3.3: [],\n"
			"		},\n"
			"		{\n"
			"		}\n"
			"	]\n"
			"}\n";
		input.pointer = data;
		input.length = sizeof(data) - 1;
		{
			size_t eaten = 0;
			luxem_bool_t result = luxem_rawread_feed(reader, &input, &eaten);
			if (!result)
			{
				struct luxem_string_t const *error = luxem_rawread_get_error(reader);
				if (error)
				{
					printf("Finished with error: ");
					luxem_print_string(error, stdout);
					printf("\n");
				}
			}
		}
	}

	luxem_rawread_destroy(reader);

	printf("Ending depth: %03lu\n", depth);

	return 0;
}

