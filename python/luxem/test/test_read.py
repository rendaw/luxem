import unittest
import itertools

import luxem

def uses_compare(method):
    def inner(self):
        method(self)
        if not self.callback_called:
            raise AssertionError('Callback not called')
    return inner

class TestRead(unittest.TestCase):
    def setUp(self):
        self.callback_called = False

    def compare(self, expected):
        def callback(got):
            self.assertEqual(isinstance(got, luxem.Typed), isinstance(expected, luxem.Typed))
            if isinstance(got, luxem.Typed):
                self.assertEqual(got.value, expected.value)
            else:
                self.assertEqual(got, expected)
            self.callback_called = True
        return callback

    @uses_compare
    def test_typed_bool(self):
        luxem.Reader().element(self.compare(True)).feed('(bool) yes')

    @uses_compare
    def test_untyped_bool(self):
        for source, expected in [
                ('False', False),
                ('false', False),
                ('No', False),
                ('no', False),
                ('0', False),
                ('True', True),
                ('true', True),
                ('Yes', True),
                ('yes', True),
                ('1', True),
                ]:
            luxem.Reader().element(self.compare(expected), luxem.bool).feed(source)

    @uses_compare
    def test_typed_int(self):
        luxem.Reader().element(self.compare(7)).feed('(int) 7')

    @uses_compare
    def test_untyped_int(self):
        luxem.Reader().element(self.compare(7), luxem.int).feed('7')
    
    @uses_compare
    def test_typed_float(self):
        luxem.Reader().element(self.compare(7)).feed('(float) 7')

    @uses_compare
    def test_untyped_float(self):
        for source, expected in [
                ('7', 7),
                ('-2.3', -2.3),
                ]:
            luxem.Reader().element(self.compare(expected), luxem.float).feed(source)
    
    @uses_compare
    def test_typed_ascii16(self):
        luxem.Reader().element(self.compare(bytearray([1, 239]))).feed('(ascii16) abop')

    @uses_compare
    def test_untyped_ascii16(self):
        for source, expected in [
                ('""', bytes(bytearray([]))),
                ('abcd', bytes(bytearray([1, 35]))),
                ]:
            luxem.Reader().element(self.compare(expected), luxem.ascii16).feed(source)
    
    @uses_compare
    def test_typed_base64(self):
        luxem.Reader().element(self.compare(bytes('sure.'))).feed('(base64) c3VyZS4=')

    @uses_compare
    def test_untyped_base64(self):
        luxem.Reader().element(self.compare(bytes('sure.')), luxem.base64).feed('c3VyZS4=')

    @uses_compare
    def test_obj_key(self):
        luxem.Reader().element(lambda obj: obj.int('key', self.compare(7))).feed('{key: (int) 7}')

    @uses_compare
    def test_array_elem(self):
        luxem.Reader().element(lambda array: array.element(self.compare(7), luxem.int)).feed('[7]')

    def test_obj_passthrough(self):
        def pass_callback(key, element):
            self.assertEqual(key, 'key')
            self.assertEqual(element.name, 'int')
            self.assertEqual(element.value, '7')
            self.callback_called = True
        luxem.Reader().element(lambda obj: obj.passthrough(pass_callback)).feed('{key: (int) 7}')
        self.assertTrue(self.callback_called)

    @uses_compare
    def test_struct_int(self):
        luxem.Reader().struct(self.compare(7)).feed('(int) 7')
    
    @uses_compare
    def test_struct_untyped(self):
        luxem.Reader().struct(self.compare('7')).feed('7')

    @uses_compare
    def test_struct_array(self):
        luxem.Reader().struct(self.compare([])).feed('[]')
    
    @uses_compare
    def test_struct_array_value(self):
        luxem.Reader().struct(self.compare([7, 9])).feed('[(int) 7, (int) 9]')
    
    @uses_compare
    def test_struct_array_depth(self):
        luxem.Reader().struct(self.compare([[7, 9]])).feed('[[(int) 7, (int) 9]]')
    
    @uses_compare
    def test_struct_object(self):
        luxem.Reader().struct(self.compare({})).feed('{}')

    @uses_compare
    def test_struct_object_key_typed(self):
        luxem.Reader().struct(self.compare({'key': 7})).feed('{key: (int) 7}')

