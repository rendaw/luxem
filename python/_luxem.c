#include <Python.h>

#include "../c/luxem_rawread.h"
#include "../c/luxem_rawwrite.h"

#include <assert.h>

/* ************************************************************************** */

static char const exception_marker;

/* Reader */
/**********/

typedef struct {
	PyObject_HEAD
	struct luxem_rawread_context_t *context;
	PyObject *object_begin;
	PyObject *object_end;
	PyObject *array_begin;
	PyObject *array_end;
	PyObject *key;
	PyObject *type;
	PyObject *primitive;
	PyThreadState *thread_state;
} Reader;

#define TRANSLATE_VOID_CALLBACK(name) \
static luxem_bool_t translate_rawread_##name(struct luxem_rawread_context_t *context, Reader *user_data) \
{ \
	PyObject *arguments = Py_BuildValue("()"); \
	if (!arguments) return luxem_false; \
	{ \
		PyObject *result = PyEval_CallObject(user_data->name, arguments); \
		Py_DECREF(arguments); \
		if (!result) \
		{ \
			luxem_rawread_get_error(context)->pointer = &exception_marker; \
			luxem_rawread_get_error(context)->length = 0; \
			return luxem_false; \
		} \
		Py_DECREF(result); \
	} \
	return luxem_true; \
}

TRANSLATE_VOID_CALLBACK(object_begin)
TRANSLATE_VOID_CALLBACK(object_end)
TRANSLATE_VOID_CALLBACK(array_begin)
TRANSLATE_VOID_CALLBACK(array_end)

#define TRANSLATE_STRING_CALLBACK(name) \
static luxem_bool_t translate_rawread_##name(struct luxem_rawread_context_t *context, Reader *user_data, struct luxem_string_t const *string) \
{ \
	PyObject *arguments = Py_BuildValue("(s#)", string->pointer, string->length); \
	if (!arguments) return luxem_false; \
	{ \
		PyObject *result = PyEval_CallObject(user_data->name, arguments); \
		Py_DECREF(arguments); \
		if (!result) \
		{ \
			luxem_rawread_get_error(context)->pointer = &exception_marker; \
			luxem_rawread_get_error(context)->length = 0; \
			return luxem_false; \
		} \
		Py_DECREF(result); \
	} \
	return luxem_true; \
}

TRANSLATE_STRING_CALLBACK(key)
TRANSLATE_STRING_CALLBACK(type)
TRANSLATE_STRING_CALLBACK(primitive)

static PyObject *Reader_new(PyTypeObject *type, PyObject *positional_args, PyObject *named_args)
{
	Reader *self = (Reader *)type->tp_alloc(type, 0);

	if (self != NULL) 
	{
		self->context = luxem_rawread_construct();
		if (!self->context)
		{
			Py_DECREF(self);
			return NULL;
		}

		{
			struct luxem_rawread_callbacks_t *callbacks = luxem_rawread_callbacks(self->context);
			callbacks->user_data = self;
			callbacks->object_begin = (luxem_rawread_void_callback_t)translate_rawread_object_begin;
			callbacks->object_end = (luxem_rawread_void_callback_t)translate_rawread_object_end;
			callbacks->array_begin = (luxem_rawread_void_callback_t)translate_rawread_array_begin;
			callbacks->array_end = (luxem_rawread_void_callback_t)translate_rawread_array_end;
			callbacks->key = (luxem_rawread_string_callback_t)translate_rawread_key;
			callbacks->type = (luxem_rawread_string_callback_t)translate_rawread_type;
			callbacks->primitive = (luxem_rawread_string_callback_t)translate_rawread_primitive;
		}

		self->object_begin = NULL;
		self->object_end = NULL;
		self->array_begin = NULL;
		self->array_end = NULL;
		self->key = NULL;
		self->type = NULL;
		self->primitive = NULL;
		self->thread_state = NULL;
	}

	return (PyObject *)self;
}

static int Reader_init(Reader *self, PyObject *positional_args, PyObject *named_args)
{
	static char *named_args_list[] = 
	{
		"object_begin", 
		"object_end", 
		"array_begin", 
		"array_end",
		"key",
		"type",
		"primitive",
		NULL
	};

	if (!PyArg_ParseTupleAndKeywords(
		positional_args, 
		named_args, 
		"OOOOOOO", 
		named_args_list, 
		&self->object_begin, 
		&self->object_end,
		&self->array_begin,
		&self->array_end,
		&self->key,
		&self->type,
		&self->primitive))
		return -1; 

	Py_INCREF(self->object_begin);
	Py_INCREF(self->object_end);
	Py_INCREF(self->array_begin);
	Py_INCREF(self->array_end);
	Py_INCREF(self->key);
	Py_INCREF(self->type);
	Py_INCREF(self->primitive);

	return 0;
}

static void Reader_dealloc(Reader *self)
{
	luxem_rawread_destroy(self->context);
	Py_XDECREF(self->object_begin);
	Py_XDECREF(self->object_end);
	Py_XDECREF(self->array_begin);
	Py_XDECREF(self->array_end);
	Py_XDECREF(self->key);
	Py_XDECREF(self->type);
	Py_XDECREF(self->primitive);
	self->ob_type->tp_free((PyObject*)self);
}

void format_context_error(struct luxem_rawread_context_t *context)
{
	if (luxem_rawread_get_error(context)->pointer != &exception_marker)
	{
		assert(luxem_rawread_get_error(context)->length > 0);
		{
			struct luxem_string_t const *error = luxem_rawread_get_error(context);
			char const *error_format = "%.*s [offset %lu]";
			int formatted_error_size = snprintf(NULL, 0, error_format, error->length, error->pointer, luxem_rawread_get_position(context));
			assert(formatted_error_size >= 0);
			if (formatted_error_size < 0)
			{
				PyErr_SetString(PyExc_ValueError, "Encountered an exception, then encountered an error while trying to format the exception.");
			}
			else
			{
				formatted_error_size += 1; /* Was returning one too small */
				{
					char *formatted_error = malloc(formatted_error_size);
					snprintf(formatted_error, formatted_error_size, error_format, error->length, error->pointer, luxem_rawread_get_position(context));
					PyErr_SetString(PyExc_ValueError, formatted_error);
					free(formatted_error);
				}
			}
		}
	}
	else
	{
		/* Pass through python exception */
		assert(luxem_rawread_get_error(context)->length == 0);
	}
}

void feed_unlock_gil(struct luxem_rawread_context_t *context, Reader *self)
{
	assert(!self->thread_state);
	self->thread_state = PyEval_SaveThread();
}

void feed_lock_gil(struct luxem_rawread_context_t *context, Reader *self)
{
	assert(self->thread_state);
	PyEval_RestoreThread(self->thread_state);
	self->thread_state = NULL;
}

static PyObject *Reader_feed(Reader *self, PyObject *positional_args, PyObject *named_args)
{
	PyObject *data;
	luxem_bool_t finish = luxem_true;
	
	static char *named_args_list[] = 
	{
		"data", 
		"finish", 
		NULL
	};

	if (!PyArg_ParseTupleAndKeywords(
		positional_args, 
		named_args, 
		"O|b", 
		named_args_list, 
		&data, 
		&finish))
		return NULL; 

	if (PyString_Check(data))
	{
		struct luxem_string_t string;
		size_t eaten = 0;
		PyString_AsStringAndSize(data, (char **)&string.pointer, &string.length);

		if (!luxem_rawread_feed(self->context, &string, &eaten, finish))
		{
			format_context_error(self->context);
			return NULL;
		}
		
		return PyInt_FromSize_t(eaten);
	}
	else if (PyFile_Check(data))
	{
		luxem_bool_t success;
		FILE *file = PyFile_AsFile(data);
		PyFile_IncUseCount((PyFileObject *)data);
		success = luxem_rawread_feed_file(self->context, file, feed_unlock_gil, feed_lock_gil);
		PyFile_DecUseCount((PyFileObject *)data);
		if (!success)
		{
			format_context_error(self->context);
			return NULL;
		}

		Py_INCREF(Py_None);
		return Py_None;
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "luxem.RawReader.feed requires a single string or file argument.");
		return NULL;
	}
}

static PyMethodDef Reader_methods[] = 
{
	{
		"feed", 
		(PyCFunction)Reader_feed, 
		METH_KEYWORDS,
		"Stream from bytes or file argument."
	},
	{NULL}
};

static PyTypeObject ReaderType = {PyObject_HEAD_INIT(NULL) 0};

static luxem_bool_t ReaderType_init(void)
{
	ReaderType.tp_name = "luxem.RawReader";
	ReaderType.tp_basicsize = sizeof(Reader);
	ReaderType.tp_dealloc = (destructor)Reader_dealloc;
	ReaderType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
	ReaderType.tp_doc = "Decodes luxem data";
	ReaderType.tp_methods = Reader_methods;
	ReaderType.tp_init = (initproc)Reader_init;
	ReaderType.tp_new = Reader_new;
	return PyType_Ready(&ReaderType) >= 0;
}

/* ************************************************************************** */

/* luxem writer */
/****************/

typedef struct {
	PyObject_HEAD
	struct luxem_rawwrite_context_t *context;
	PyObject *write;
	PyObject *file;
} Writer;

static luxem_bool_t translate_rawwrite_write(struct luxem_rawwrite_context_t *context, Writer *user_data, struct luxem_string_t const *string)
{
	PyObject *arguments = Py_BuildValue("(s#)", string->pointer, string->length);
	if (!arguments) return luxem_false;
	{
		PyObject *result = PyEval_CallObject(user_data->write, arguments);
		Py_DECREF(arguments);
		if (!result)
		{
			luxem_rawwrite_get_error(context)->pointer = &exception_marker;
			luxem_rawwrite_get_error(context)->length = 0;
			return luxem_false;
		}
		Py_DECREF(result);
	}
	return luxem_true;
}

static PyObject *Writer_new(PyTypeObject *type, PyObject *positional_args, PyObject *named_args)
{
	Writer *self = (Writer *)type->tp_alloc(type, 0);

	if (self != NULL) 
	{
		self->context = luxem_rawwrite_construct();
		if (!self->context)
		{
			Py_DECREF(self);
			return NULL;
		}

		self->write = NULL;
		self->file = NULL;
	}

	return (PyObject *)self;
}

static int Writer_init(Writer *self, PyObject *positional_args, PyObject *named_args)
{
	luxem_bool_t pretty = luxem_false, use_spaces = luxem_false;
	int indent_multiple = 1;

	static char *named_args_list[] = 
	{
		"write_callback", 
		"write_file",
		"pretty", 
		"use_spaces", 
		"indent_multiple",
		NULL
	};

	if (!PyArg_ParseTupleAndKeywords(
		positional_args, 
		named_args, 
		"|OObbi", 
		named_args_list, 
		&self->write, 
		&self->file,
		&pretty,
		&use_spaces,
		&indent_multiple))
		return -1; 

	if (self->write)
	{
		Py_INCREF(self->write);
		luxem_rawwrite_set_write_callback(self->context, (luxem_rawwrite_write_callback_t)translate_rawwrite_write, self);
	}

	if (self->file)
	{
		Py_INCREF(self->file);
		luxem_rawwrite_set_file_out(self->context, PyFile_AsFile(self->file));
	}

	if (!self->write && !self->file)
		luxem_rawwrite_set_buffer_out(self->context);

	if (pretty)
		luxem_rawwrite_set_pretty(self->context, use_spaces ? ' ' : '\t', indent_multiple);

	return 0;
}

static void Writer_dealloc(Writer *self)
{
	luxem_rawwrite_destroy(self->context);
	Py_XDECREF(self->write);
	Py_XDECREF(self->file);
	self->ob_type->tp_free((PyObject*)self);
}

static PyObject *translate_void_method(Writer *self, luxem_bool_t (*method)(struct luxem_rawwrite_context_t *))
{
	if (!method(self->context))
	{
		if (luxem_rawwrite_get_error(self->context)->pointer != &exception_marker)
		{
			struct luxem_string_t const *error = luxem_rawwrite_get_error(self->context);
			assert(error->length > 0);
			PyErr_SetObject(PyExc_ValueError, PyString_FromStringAndSize(error->pointer, error->length));
		}
		else
		{
			/* Pass through python exception */
			assert(luxem_rawwrite_get_error(self->context)->length == 0);
		}
		return NULL;
	}
	
	Py_INCREF((PyObject *)self);
	return (PyObject *)self;
}

static PyObject *Writer_object_begin(Writer *self) { return translate_void_method(self, luxem_rawwrite_object_begin); }
static PyObject *Writer_object_end(Writer *self) { return translate_void_method(self, luxem_rawwrite_object_end); }
static PyObject *Writer_array_begin(Writer *self) { return translate_void_method(self, luxem_rawwrite_array_begin); }
static PyObject *Writer_array_end(Writer *self) { return translate_void_method(self, luxem_rawwrite_array_end); }

static PyObject *translate_string_method(Writer *self, PyObject *positional_args, luxem_bool_t (*method)(struct luxem_rawwrite_context_t *, struct luxem_string_t const *), char const *badargs) 
{ 
	PyObject *argument; 
	
	if (!PyArg_ParseTuple( 
		positional_args, 
		"O", 
		&argument)) 
		return NULL; 
	
	if (PyString_Check(argument)) 
	{ 
		struct luxem_string_t string; 
		PyString_AsStringAndSize(argument, (char **)&string.pointer, &string.length); 
		
		if (!method(self->context, &string)) 
		{ 
			if (luxem_rawwrite_get_error(self->context)->pointer != &exception_marker) 
			{ 
				struct luxem_string_t const *error = luxem_rawwrite_get_error(self->context); 
				assert(error->length > 0); 
				PyErr_SetObject(PyExc_ValueError, PyString_FromStringAndSize(error->pointer, error->length)); 
			} 
			else 
			{ 
				/* Pass through python exception */ 
				assert(luxem_rawwrite_get_error(self->context)->length == 0); 
			} 
			return NULL; 
		} 
	} 
	else 
	{ 
		PyErr_SetString(PyExc_TypeError, badargs); 
		return NULL; 
	} 
	
	Py_INCREF((PyObject *)self); 
	return (PyObject *)self; 
}

static PyObject *Writer_key(Writer *self, PyObject *positional_args) 
	{ return translate_string_method(self, positional_args, luxem_rawwrite_key, "luxem.RawWriter.key requires a single string argument."); }
static PyObject *Writer_type(Writer *self, PyObject *positional_args) 
	{ return translate_string_method(self, positional_args, luxem_rawwrite_type, "luxem.RawWriter.type requires a single string argument."); }
static PyObject *Writer_primitive(Writer *self, PyObject *positional_args)
	{ return translate_string_method(self, positional_args, luxem_rawwrite_primitive, "luxem.RawWriter.primitive requires a single string argument."); }

static PyObject *Writer_dump(Writer *self)
{
	if (self->write || self->file)
	{
		PyErr_SetString(PyExc_TypeError, "luxem.RawWriter.dump can only be used if not using a custom serialize callback for serializing to file.");
		return NULL;
	}
	else
	{
		struct luxem_string_t *rendered = luxem_rawwrite_buffer_render(self->context);
		if (!rendered)
		{
				struct luxem_string_t const *error = luxem_rawwrite_get_error(self->context); 
				assert(error->length > 0); 
				PyErr_SetObject(PyExc_ValueError, PyString_FromStringAndSize(error->pointer, error->length)); 
				return NULL;
		}

		PyObject *out = PyString_FromStringAndSize(rendered->pointer, rendered->length);
		free(rendered);
		return out;
	}
}

static PyMethodDef Writer_methods[] = 
{
	{"object_begin", (PyCFunction)Writer_object_begin, METH_NOARGS, "Start a new object."},
	{"object_end", (PyCFunction)Writer_object_end, METH_NOARGS, "End current object."},
	{"array_begin", (PyCFunction)Writer_array_begin, METH_NOARGS, "Start a new array."},
	{"array_end", (PyCFunction)Writer_array_end, METH_NOARGS, "End current array."},
	{"key", (PyCFunction)Writer_key, METH_VARARGS, "Write a key."},
	{"type", (PyCFunction)Writer_type, METH_VARARGS, "Write a type."},
	{"primitive", (PyCFunction)Writer_primitive, METH_VARARGS, "Write a primitive."},
	{"dump", (PyCFunction)Writer_dump, METH_NOARGS, "If serializing to a buffer, returns all rendered data so far."},
	{NULL}
};

static PyTypeObject WriterType = {PyObject_HEAD_INIT(NULL) 0};

static luxem_bool_t WriterType_init(void)
{
	WriterType.tp_name = "luxem.RawWriter";
	WriterType.tp_basicsize = sizeof(Writer);
	WriterType.tp_dealloc = (destructor)Writer_dealloc;
	WriterType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
	WriterType.tp_doc = "Encodes luxem data";
	WriterType.tp_methods = Writer_methods;
	WriterType.tp_init = (initproc)Writer_init;
	WriterType.tp_new = Writer_new;
	return PyType_Ready(&WriterType) >= 0;
}

/* ************************************************************************** */

/* luxem module + module entry point */
/*************************************/

static PyObject *translate_to_from_ascii16(PyObject *self, PyObject *positional_args, struct luxem_string_t const *(*function)(struct luxem_string_t const *, struct luxem_string_t *))
{
	PyObject *argument;
	if (!PyArg_ParseTuple(
		positional_args, 
		"O", 
		&argument))
		return NULL; 

	if (PyString_Check(argument))
	{
		struct luxem_string_t string;
		struct luxem_string_t error;
		struct luxem_string_t const *out;
		PyObject *out_string;
		PyString_AsStringAndSize(argument, (char **)&string.pointer, &string.length);
		out = function(&string, &error);
		if (!out)
		{
			PyErr_SetObject(PyExc_ValueError, PyString_FromStringAndSize(error.pointer, error.length)); 
			return NULL;
		}
		out_string = PyString_FromStringAndSize(out->pointer, out->length);
		free((void *)out);
		return out_string;
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "A single string argument is required.");
		return NULL;
	}
}

static PyObject *translate_to_ascii16(PyObject *self, PyObject *positional_args)
	{ return translate_to_from_ascii16(self, positional_args, luxem_to_ascii16); }

static PyObject *translate_from_ascii16(PyObject *self, PyObject *positional_args)
	{ return translate_to_from_ascii16(self, positional_args, luxem_from_ascii16); }

static PyMethodDef luxem_methods[] = 
{
	{"to_ascii16", (PyCFunction)translate_to_ascii16, METH_VARARGS, "Encode ascii16."},
	{"from_ascii16", (PyCFunction)translate_from_ascii16, METH_VARARGS, "Decode ascii16."},
	{NULL}
};

#ifndef PyMODINIT_FUNC	/* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif
PyMODINIT_FUNC init_luxem(void) 
{
	PyObject* module;

	if (!ReaderType_init()) return;
	if (!WriterType_init()) return;

	module = Py_InitModule3("_luxem", luxem_methods, "luxem C API internal-use module.");

	Py_INCREF(&ReaderType);
	PyModule_AddObject(module, "Reader", (PyObject *)&ReaderType);
	PyModule_AddObject(module, "Writer", (PyObject *)&WriterType);
}

