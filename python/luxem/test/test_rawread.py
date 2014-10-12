import unittest
import itertools
import io

import luxem

long_input = '''

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

long_input_sequence = [
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

class TestRawRead(unittest.TestCase):
    def setUp(self):
        self.sequence = []
        self.reader = luxem.RawReader(
                object_begin=lambda: self.sequence.append(('object begin', None)),
                object_end=lambda: self.sequence.append(('object end', None)),
                array_begin=lambda: self.sequence.append(('array begin', None)),
                array_end=lambda: self.sequence.append(('array end', None)),
                key=lambda data: self.sequence.append(('key', data)),
                type=lambda data: self.sequence.append(('type', data)),
                primitive=lambda data: self.sequence.append(('primitive', data)),
        )

    def compare(self, expected_sequence):
        for got, expected in itertools.izip_longest(enumerate(self.sequence), enumerate(expected_sequence)):
            self.assertEqual(got, expected)

    def test_empty(self):
        self.reader.feed('')
        self.compare([])
    
    def test_untyped(self):
        self.reader.feed('7')
        self.compare([('primitive', '7')])
    
    def test_untyped_comma(self):
        self.reader.feed('7, ')
        self.compare([('primitive', '7')])

    def test_untyped_words(self):
        self.reader.feed('"yodel minister"')
        self.compare([('primitive', 'yodel minister')])
    
    def test_untyped_empty(self):
        self.reader.feed('""')
        self.compare([('primitive', '')])

    def test_untyped_word_escapes(self):
        self.reader.feed('goob\\er')
        self.compare([('primitive', 'goober')])

    def test_untyped_words_escapes(self):
        self.reader.feed('"\\" is \\\\ apple"')
        self.compare([('primitive', '" is \\ apple')])

    def test_untyped_nofinish(self):
        self.reader.feed('7', finish=False)
        self.compare([])

    def test_typed(self):
        self.reader.feed('(int) 7')
        self.compare([('type', 'int'), ('primitive', '7')])
    
    def test_object(self):
        self.reader.feed('{}')
        self.compare([('object begin', None), ('object end', None)])
    
    def test_key_object(self):
        self.reader.feed('{q:7}')
        self.compare([('object begin', None), ('key', 'q'), ('primitive', '7'), ('object end', None)])

    def test_close_object_nofinish(self):
        self.reader.feed('{}', finish=False)
        self.compare([('object begin', None), ('object end', None)])

    def test_array(self):
        self.reader.feed('[]')
        self.compare([('array begin', None), ('array end', None)])

    def test_basic(self):
        read_length = self.reader.feed(long_input)
        self.assertEqual(read_length, len(long_input))
        self.compare(long_input_sequence)

    def test_break_whitespace(self):
        read_length = self.reader.feed(' ', finish=False)
        self.assertEqual(read_length, 0)
        read_length = self.reader.feed('  a')
        self.assertEqual(read_length, 3)
        self.compare([('primitive', 'a')])

class TestRawReadFile(TestRawRead):
    def setUp(self):
        super(TestRawReadFile, self).setUp()
        self.data = open('test_temp_file', 'r+')
        self.data.truncate()

    def tearDown(self):
        self.data.close()

    def test_test_setup(self):
        self.data.write('wizzly gumb')
        self.data.seek(0)
        self.assertEqual(self.data.read(), 'wizzly gumb')
    
    def test_empty(self):
        self.reader.feed(self.data)
        self.compare([])
    
    def test_untyped(self):
        self.data.write('7')
        self.data.seek(0)
        self.reader.feed(self.data)
        self.compare([('primitive', '7')])

    def test_untyped_comma(self):
        self.data.write('7, ')
        self.data.seek(0)
        self.reader.feed(self.data)
        self.compare([('primitive', '7')])
    
    def test_untyped_words(self):
        self.data.write('"yodel minister"')
        self.data.seek(0)
        self.reader.feed(self.data)
        self.compare([('primitive', 'yodel minister')])

    def test_object(self):
        self.data.write('{}')
        self.data.seek(0)
        self.reader.feed(self.data)
        self.compare([('object begin', None), ('object end', None)])
    
    def test_array(self):
        self.data.write('[]')
        self.data.seek(0)
        self.reader.feed(self.data)
        self.compare([('array begin', None), ('array end', None)])
    
    def test_basic(self):
        self.data.write(long_input)
        self.data.seek(0)
        self.reader.feed(self.data)
        self.compare(long_input_sequence)

    def test_long_file(self):
        compare_sequence = []
        for index in range(100):
            string = 'index {}, plus some junk: {}'.format(index, '*' * 30)
            self.data.write('"{}",'.format(string))
            compare_sequence.append(('primitive', string))
        self.data.seek(0)
        self.reader.feed(self.data)
        self.compare(compare_sequence)
    
class TestRawRead2(unittest.TestCase):
    def test_callback_void_except(self):
        class TestError(Exception):
            pass
        def callback():
            raise TestError()
        reader = luxem.RawReader(
                object_begin=callback,
                object_end=None,
                array_begin=None,
                array_end=None,
                key=None,
                type=None,
                primitive=None
        )
        self.assertRaises(TestError, reader.feed, '{}')
