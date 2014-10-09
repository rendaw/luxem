import base64

import _luxem
import struct

def process_typed_bool(element):
    return False if element.lower() in ['0', 'false', 'no'] else True

def process_bool(element):
    if isinstance(element, struct.Typed):
        if element.name not in ['bool']:
            raise ValueError('Expected bool but got typed {}'.format(element.name))
        return process_typed_bool(element.value)
    else:
        return process_typed_bool(element)

def process_typed_int(element):
    return int(element)

def process_int(element):
    if isinstance(element, struct.Typed):
        if element.name not in ['int']:
            raise ValueError('Expected int but got typed {}'.format(element.name))
        return process_typed_int(element.value)
    else:
        return process_typed_int(element)

def process_typed_float(element):
    return float(element)

def process_float(element):
    if isinstance(element, struct.Typed):
        if element.name not in ['float']:
            raise ValueError('Expected float but got typed {}'.format(element.name))
        return process_typed_float(element.value)
    else:
        return process_typed_float(element)

def process_typed_string(element):
    return element.value

def process_string(element):
    if isinstance(element, struct.Typed):
        if element.name not in ['string']:
            raise ValueError('Expected string but got typed {}'.format(element.name))
        process_typed_string(element.value)
    else:
        process_typed_string(element)

def process_typed_base64(element):
    return base64.b64decode(element)

def process_base64(element):
    if isinstance(element, struct.Typed):
        return process_typed_base64(element.value)
    else:
        return process_typed_base64(element)

def process_typed_ascii16(element):
    return _luxem.from_ascii16(element)

def process_ascii16(element):
    if isinstance(element, struct.Typed):
        return process_typed_ascii16(element.value)
    else:
        return process_typed_ascii16(element)

def process_bytes(element):
    if isinstance(element, struct.Typed):
        if element.name == 'ascii16':
            return process_typed_ascii16(element.value)
        elif element.name == 'base64':
            return process_typed_base64(element.value)
        else:
            raise ValueError('Expected bytes but got typed {}'.format(element.name))
    else:
        raise ValueError('Expected types but no value type specified.')

def process_object(element):
    if isinstance(element, struct.Typed):
        if not isinstace(element.value, dict):
            raise ValueError('Expected object but got {}'.format(type(element.value)))
    else:
        if not isinstance(element, dict):
            raise ValueError('Expected object but got {}'.format(type(element)))
    return element

def process_array(element):
    if isinstance(element, struct.Typed):
        if not isinstace(element.value, dict):
            raise ValueError('Expected array but got {}'.format(type(element.value)))
    else:
        if not isinstance(element, dict):
            raise ValueError('Expected array but got {}'.format(type(element)))
    return element

_process_any_lookup = {
    'bool': process_typed_bool,
    'int': process_typed_int,
    'float': process_typed_float,
    'string': process_typed_string,
    'ascii16': process_typed_ascii16,
    'base64': process_typed_base64,
    'bytes': process_bytes,
    'array': process_array,
    'object': process_object,
}

def process_any(element):
    if isinstance(element, struct.Typed):
        processor = _process_any_lookup.get(element.name)
        if processor:
            return processor(element.value)
    return element

