import unittest
import itertools

import luxem

class TestRawRead(unittest.TestCase):
    def test_basic(self):
        sequence = []
        reader = luxem.Reader(
                object_begin=lambda: sequence.append(('object begin', None)),
                object_end=lambda: sequence.append(('object end', None)),
                array_begin=lambda: sequence.append(('array begin', None)),
                array_end=lambda: sequence.append(('array end', None)),
                key=lambda data: sequence.append(('key', data)),
                type=lambda data: sequence.append(('type', data)),
                primitive=lambda data: sequence.append(('primitive', data)),
        )
        input_data = b'''

{
       key1: val1,key1.5:val1.5,
       \"key2 with spaces\": \"val2 with spaces\",
       key3: (type3) val3,
       key4:(type4)\"val4 with spaces\",
       key5: [
               val5.1,
               \"val5.2\",
               (type5.3) {
                       val5.3.3: [],
               },
               {
               }
       ]
}
'''
        read_length = reader.feed(input_data)
        self.assertEqual(read_length, len(input_data))
        expected_sequence = [
            ('object begin', None),
            ('key', 'key1'),
            ('primitive', 'val1'),
            ('key', 'key1.5'),
            ('primitive', 'val1.5'),
            ('key', 'key2 with spaces'),
            ('primitive', 'val2 with spaces'),
            ('key', 'key3'),
            ('type', 'type3'),
            ('primitive', 'val3'),
            ('key', 'key4'),
            ('type', 'type4'),
            ('primitive', 'val4 with spaces'),
            ('key', 'key5'),
            ('array begin', None),
            ('primitive', 'val5.1'),
            ('primitive', 'val5.2'),
            ('type', 'type5.3'),
            ('object begin', None),
            ('key', 'val5.3.3'),
            ('array begin', None),
            ('array end', None),
            ('object end', None),
            ('object begin', None),
            ('object end', None),
            ('array end', None),
            ('object end', None),
        ]
        for got, expected in itertools.izip_longest(enumerate(sequence), enumerate(expected_sequence)):
            self.assertEqual(got, expected)
