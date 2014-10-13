import unittest
import itertools

import luxem

long_text = '''{
    key1: val1,
    key1.5: val1.5,
    key3: (type3) val3,
    key4: (type4) "val4 with spaces",
    key5: [
        val5.1,
        val5.2,
        (type5.3) {
            val5.3.3: [
            ],
        },
        {
        },
    ],
},
'''

def write_long_sequence(writer):
    (
        writer.object_begin()
            .key('key1').primitive('val1')
            .key('key1.5').primitive('val1.5')
            .key('key3').type('type3').primitive('val3')
            .key('key4').type('type4').primitive('val4 with spaces')
            .key('key5').array_begin()
                .primitive('val5.1')
                .primitive('val5.2')
                .type('type5.3').object_begin()
                    .key('val5.3.3').array_begin().array_end()
                .object_end()
                .object_begin()
                .object_end()
            .array_end()
        .object_end()
    )

class TestRawWrite(unittest.TestCase):
    def setUp(self):
        self.sequence = []
        self.writer = luxem.RawWriter(
            write_callback=lambda text: self.sequence.append(text),
            pretty=True,
            use_spaces=True,
            indent_multiple=4
        )

    def compare(self, string):
        output = ''.join(self.sequence)
        #print('Got: {}'.format(output))
        #print('Exp: {}'.format(string))
        self.assertEqual(output, string)

    def test_string(self):
        self.writer.primitive('primitive')
        self.compare('primitive,\n')

    def test_string_spaces(self):
        self.writer.primitive('has spaces')
        self.compare('"has spaces",\n')

    def test_string_quotes(self):
        self.writer.primitive('"')
        self.compare('"\\"",\n')

    def test_type(self):
        self.writer.type('type').primitive('value')
        self.compare('(type) value,\n')

    def test_type_with_spaces(self):
        self.writer.type('has spaces').primitive('value')
        self.compare('(has spaces) value,\n')

    def test_object(self):
        self.writer.object_begin().object_end()
        self.compare('{\n},\n')
    
    def test_typed_object(self):
        self.writer.type('type').object_begin().object_end()
        self.compare('(type) {\n},\n')

    def test_object_object(self):
        self.writer.object_begin().key('key').object_begin().object_end().object_end()
        self.compare('{\n    key: {\n    },\n},\n')

    def test_object_one_element(self):
        self.writer.object_begin().key('key').primitive('primitive').object_end()
        self.compare('{\n    key: primitive,\n},\n')
    
    def test_object_one_typed_element(self):
        self.writer.object_begin().key('key').type('type').primitive('primitive').object_end()
        self.compare('{\n    key: (type) primitive,\n},\n')
    
    def test_array(self):
        self.writer.array_begin().array_end()
        self.compare('[\n],\n')
    
    def test_object_array(self):
        self.writer.object_begin().key('key').array_begin().array_end().object_end()
        self.compare('{\n    key: [\n    ],\n},\n')
    
    def test_typed_array(self):
        self.writer.type('type').array_begin().array_end()
        self.compare('(type) [\n],\n')
    
    def test_array_one_element(self):
        self.writer.array_begin().primitive('primitive').array_end()
        self.compare('[\n    primitive,\n],\n')
        
    def test_basic(self):
        write_long_sequence(self.writer)
        self.compare(long_text)

class TestRawWriteBuffer(unittest.TestCase):
    def setUp(self):
        self.writer = luxem.RawWriter(
            pretty=True,
            use_spaces=True,
            indent_multiple=4
        )
    def compare(self, string):
        output = self.writer.dump()
        #print('Got: {}'.format(output))
        #print('Exp: {}'.format(string))
        self.assertEqual(output, string)

    def test_string(self):
        self.writer.primitive('primitive')
        self.compare('primitive,\n')
    
    def test_basic(self):
        write_long_sequence(self.writer)
        self.compare(long_text)

class TestRawWriteFile(unittest.TestCase):
    def setUp(self):
        self.file = open('test_temp_file', 'r+')
        self.file.truncate()
        self.writer = luxem.RawWriter(
            write_file=self.file,
            pretty=True,
            use_spaces=True,
            indent_multiple=4
        )
    def compare(self, string):
        self.file.seek(0)
        output = self.file.read()
        #print('Got: {}'.format(output))
        #print('Exp: {}'.format(string))
        self.assertEqual(output, string)

    def test_string(self):
        self.writer.primitive('primitive')
        self.compare('primitive,\n')
    
    def test_basic(self):
        write_long_sequence(self.writer)
        self.compare(long_text)

