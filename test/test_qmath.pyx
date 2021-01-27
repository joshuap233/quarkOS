# cython: c_string_type=unicode, c_string_encoding=utf8
# 设置自动编码,不要删除上面的注释

cimport qmath
import random
import tool
import math


class Test(tool.TestCase):
    max_int = 1000
    def random_float(self):
        return random.randint(-self.max_int, self.max_int) + random.random()

    def test_ceilf(self):
        rand_float = self.random_float()
        cdef double s = rand_float
        self.assertEqual(qmath.q_ceilf(s), math.ceil(rand_float))
        self.assertEqual(qmath._q_ceilf(s), math.ceil(rand_float))


    # def test_floor(self):
    #     rand_float = random.random() * self.max_int
    #     cdef double s = rand_float
    #     self.assertEqual(qmath.q_floorf(s), math.floor(rand_float))

    def test_divUc(self):
        i1 = random.randint(0, self.max_int)
        i2 = random.randint(0, self.max_int)
        self.assertEqual(qmath.divUc(i1, i2), math.ceil(i1 / i2))



Test().run()
