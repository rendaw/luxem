import base64
import zlib
import StringIO
import random

import luxem

source = ''.join(chr(random.randint(0, 255)) for index in range(1024 * 1024))

print('Size in: {}'.format(len(source)))

ascii16 = luxem.to_ascii16(source)
print('Size ascii16: {}'.format(len(ascii16)))
print('Size zipped ascii16: {}'.format(len(zlib.compress(ascii16))))
del ascii16

base64_data = base64.b64encode(source)
print('Size base64: {}'.format(len(base64_data)))
print('Size zipped base64: {}'.format(len(zlib.compress(base64_data))))
del base64_data

