import unittest

import luxem

class TestWrite(unittest.TestCase):
    def setUp(self):
        sequence = []
        self.writer = luxem.Writer(
            lambda text: sequence.append(text),
        )
        def compare(string):
            output = ''.join(sequence)
            #print('Got: {}'.format(output))
            #print('Exp: {}'.format(string))
            self.assertEqual(output, string)
        self.compare = compare

    def test_int(self):
        self.writer.value(7)
        self.compare('7,')

    def test_typed_int(self):
        self.writer.value(luxem.Typed('int', 7))
        self.compare('(int)7,')

    def test_float(self):
        self.writer.value(7.9)
        self.compare('7.9,')

    def test_string(self):
        self.writer.value('hey')
        self.compare('hey,')

    def test_spaced_string(self):
        self.writer.value('hey glovebox')
        self.compare('"hey glovebox",')

    def test_escaped_string(self):
        self.writer.value('do\\g')
        self.compare('do\\g,')

    def test_escaped_spaced_string(self):
        self.writer.value('hey \\glovebox')
        self.compare('"hey \\\\glovebox",')

    def test_ascii16(self):
        self.writer.value(luxem.Typed('ascii16', str(bytearray([1, 239]))))
        self.compare('(ascii16)abop,')

    def test_base64(self):
        self.writer.value(luxem.Typed('base64', 'sure.'))
        self.compare('(base64)c3VyZS4=,')

    def test_object(self):
        self.writer.value({})
        self.compare('{},')
    
    def test_object_keys(self):
        self.writer.value({'dig': 'wombat', 'fig': 'combat'})
        self.compare('{dig:wombat,fig:combat,},')

    def test_object_key_object(self):
        self.writer.value({'elebent': {}})
        self.compare('{elebent:{},},')
    
    def test_object_key_array(self):
        self.writer.value({'elebent': []})
        self.compare('{elebent:[],},')
    
    def test_array(self):
        self.writer.value([])
        self.compare('[],')
    
    def test_array_elements(self):
        self.writer.value(['flag', 'nutter'])
        self.compare('[flag,nutter,],')

    def test_array_array(self):
        self.writer.value([[]])
        self.compare('[[],],')

    def test_array_object(self):
        self.writer.value([{}])
        self.compare('[{},],')

