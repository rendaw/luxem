import base64

import _luxem
import struct

class _ArrayElement(object):
    def __init__(self, item):
        self.item_iter = iter(item)

    def step(self, writer, stack):
        try:
            next_child = next(self.item_iter)
            writer._process(stack, next_child)
            return True
        except StopIteration:
            writer.array_end()
            return False

class _ObjectElement(object):
    def __init__(self, item):
        self.item_iter = item.iteritems()

    def step(self, writer, stack):
        try:
            next_child = next(self.item_iter)
            writer.key(next_child[0])
            writer._process(stack, next_child[1])
            return True
        except StopIteration:
            writer.object_end()
            return False

class Writer(_luxem.Writer):
    def _process(self, stack, item):
        if hasattr(item, 'iteritems'):
            self.object_begin()
            stack.append(_ObjectElement(item))
        elif hasattr(item, '__iter__'):
            self.array_begin()
            stack.append(_ArrayElement(item))
        elif isinstance(item, struct.Typed):
            self.type(item.name)
            if item.name == 'ascii16':
                self.primitive(_luxem.to_ascii16(item.value))
            elif item.name == 'base64':
                self.primitive(base64.b64encode(item.value))
            elif item.name in ['int', 'float', 'string']:
                self.primitive(unicode(item.value).encode('utf-8'))
            else:
                self._process(stack, item.value)
        else:
            self.primitive(unicode(item).encode('utf-8'))

    def value(self, data):
        stack = []
        self._process(stack, data)
        while stack:
            while stack[-1].step(self, stack):
                pass
            stack.pop()
        return self
