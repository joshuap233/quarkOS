# cython: c_string_type=unicode, c_string_encoding=utf8
# 设置自动编码,不要删除上面的注释

cimport qlib
import random
import tool
from libc.stdlib cimport malloc, free
from libc.stdint cimport uint32_t


class Test(tool.TestCase):
    max_int = 2 ** 32 - 1

    def test_itoa(self):
        cdef char *string = <char*> malloc(33)
        rand_int = random.randint(0, self.max_int)
        qlib.q_itoa(rand_int, string)
        self.assertEqual(string, str(rand_int))
        free(string)

    def test_hex(self):
        rand_int = random.randint(0, self.max_int)
        cdef char *string = <char*> malloc(33)
        qlib.hex(rand_int, string)
        self.assertEqual(string, hex(rand_int)[2:])

    def test_hex_limit(self):
        cdef char *string = <char*> malloc(33)
        it = 0
        qlib.hex(it, string)
        self.assertEqual(string, hex(it)[2:])

    # def test_generate_mask(self):
    #     def generate_mask(n):
    #         return int('1' * n, 2) if n > 0 else 0
    #     rand_int = random.randint(0, 64)
    #     if rand_int > 32:
    #         self.assertEqual(qlib.generate_mask(rand_int), 0)
    #     else:
    #         self.assertEqual(qlib.generate_mask(rand_int), generate_mask(rand_int))



Test().run()
