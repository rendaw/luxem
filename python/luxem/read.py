import _luxem
import struct
from processors import *

def _read_struct_element_object(element, callback, type_name=None):
    value = {}
    if type_name is None:
        out = value
    else:
        out = struct.Typed(type_name)
        out.value = value
    def object_callback(key, subelement):
        def subcallback(substruct):
            value[key] = substruct
        _read_struct(subelement, subcallback)
    element.passthrough(object_callback)
    element.finished(lambda: callback(out))

def _read_struct_element_array(element, callback, type_name=None):
    value = []
    if type_name is None:
        out = value
    else:
        out = struct.Typed(type_name)
        out.value = value
    element.element(
        lambda subelement: _read_struct(
            subelement, 
            lambda substruct: value.append(substruct)
        )
    )
    element.finished(lambda: callback(out))

def _read_struct(element, callback):
    if isinstance(element, struct.Typed):
        if isinstance(element, Reader.Object):
            _read_struct_element_object(element.value, callback, element.name)
        elif isinstance(element, Reader.Array):
            _read_struct_element_array(element.value, callback, element.name)
        else:
            callback(process_any(element))
    else:
        if isinstance(element, Reader.Object):
            _read_struct_element_object(element, callback)
        elif isinstance(element, Reader.Array):
            _read_struct_element_array(element, callback)
        else:
            callback(process_any(element))

class Reader(_luxem.Reader):
    class Object(object):
        def __init__(self):
            self._passthrough_callback = None
            self._finish_callback = None
            self._callbacks = {}

        def _process(self, element, key):
            if self._passthrough_callback:
                self._passthrough_callback(key, element)
                return
            callback = self._callbacks.get(key)
            if not callback:
                return
            callback(element)

        def _finish(self):
            if self._finish_callback:
                self._finish_callback()

        def bool(self, key, callback):
            self._callbacks[key] = lambda element: callback(process_bool(element))

        def int(self, key, callback):
            self._callbacks[key] = lambda element: callback(process_int(element))
        
        def float(self, key, callback):
            self._callbacks[key] = lambda element: callback(process_float(element))
        
        def string(self, key, callback):
            self._callbacks[key] = lambda element: callback(process_string(element))
        
        def bytes(self, key, callback):
            self._callbacks[key] = lambda element: callback(process_bytes(element))

        def ascii16(self, key, callback):
            self._callbacks[key] = lambda element: callback(process_ascii16(element))
        
        def base64(self, key, callback):
            self._callbacks[key] = lambda element: callback(process_base64(element))
        
        def object(self, key, callback):
            self._callbacks[key] = lambda element: callback(process_object(element))

        def array(self, key, callback):
            self._callbacks[key] = lambda element: process_array(element, callback)

        def element(self, key, callback, processor=None):
            def element_callback(element):
                if processor:
                    callback(processor(element))
                else:
                    callback(process_any(element))
            self._callbacks[key] = element_callback

        def struct(self, key, callback):
            self._callbacks[key] = lambda element: _read_struct(element, callback)

        def passthrough(self, callback):
            self._passthrough_callback = callback
        
        def finished(self, callback):
            self._finish_callback = callback

    class Array(object):
        def __init__(self):
            self._processor = None
            self._callback = None
            self._finish_callback = None

        def _process(self, element, **kwargs):
            if not self._callback:
                return
            if self._processor:
                self._callback(self._processor(element))
            else:
                self._callback(process_any(element))

        def _finish(self):
            if self._finish_callback:
                self._finish_callback()

        def element(self, callback, processor=None):
            if self._callback:
                raise ValueError('Callback already set!')
            self._processor = processor
            self._callback = callback

        def struct(self, callback):
            if self._callback:
                raise ValueError('Callback already set!')
            self._callback = lambda element: _read_struct(element, callback)

        def finished(self, callback):
            self._finish_callback = callback

    def __init__(self, translate_types=True):
        self._stack = [Reader.Array()]
        self._current_key = None
        self._current_type = None

        super(Reader, self).__init__(
            object_begin=self._object_begin,
            object_end=self._pop,
            array_begin=self._array_begin,
            array_end=self._pop,
            key=self._key,
            type=self._type,
            primitive=self._primitive,
        )
        
    def element(self, callback, processor=None):
        self._stack[0].element(callback, processor)
        return self
        
    def struct(self, callback):
        self._stack[0].struct(callback)
        return self

    def _process(self, element):
        if self._current_type is not None:
            element = struct.Typed(self._current_type, element)
            self._current_type = None
        if self._stack:
            self._stack[-1]._process(element, key=self._current_key)
        self._current_key = None

    def _object_begin(self):
        element = Reader.Object()
        self._process(element)
        self._stack.append(element)

    def _array_begin(self):
        element = Reader.Array()
        self._process(element)
        self._stack.append(element)

    def _key(self, data):
        self._current_key = data

    def _type(self, data):
        self._current_type = data

    def _primitive(self, data):
        self._process(data)

    def _pop(self):
        self._stack[-1]._finish()
        self._stack.pop()

