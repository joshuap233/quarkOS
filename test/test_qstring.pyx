# cython: c_string_type=unicode, c_string_encoding=utf8
# 设置自动编码, 不要删除上面的注释

from libc.stdlib cimport malloc, free
from libc.string cimport strcpy, strcmp
import random
cimport qstring
import tool


class Test(tool.TestCase):
    max_uint16 = 2 ** 16 - 2
    max_int = 2 ** 31 - 1
    min_int = -2 ** 31
    max_uint64 = 2 ** 64 - 1

    def test_strlen(self):
        string = tool.random_str()
        self.assertEqual(qstring.q_strlen(string), len(string))

    def test_strcat(self):
        tail = "tail"
        string = tool.random_str()
        cdef char*s = <char *> malloc(len(string) + 1 + len(tail))
        strcpy(s, string)
        qstring.q_strcat(s, tail)
        self.assertEqual(s, string + tail)
        free(s)

    def test_strncat(self):
        tail = "tail"
        string = tool.random_str()
        rani = random.randint(0, len(tail))
        cdef char*s = <char *> malloc(len(string) + 1 + len(tail))
        strcpy(s, string)
        qstring.q_strncat(s, tail, rani)
        self.assertEqual(s, string + tail[:rani])
        free(s)

    def test_strcmp(self):
        string = tool.random_str()
        string2 = tool.random_str()

        self.assertTrue(qstring.q_strcmp(string, string))
        self.assertEqual(qstring.q_strcmp(string, string2), string == string2)

    def test_memcpy(self):
        def rand_int():
            return random.randint(self.min_int, self.max_int)
        # 测试字符复制
        rs = tool.random_str()
        cdef char*dest = <char *> malloc(len(rs) + 1)
        cdef char*src = rs

        qstring.q_memcpy(dest, src, len(rs) + 1)
        self.assertEqual(strcmp(dest, src), 0)
        free(dest)

        # 测试整数复制
        rn = rand_int()
        cdef int dest1, src1 = rn
        qstring.q_memcpy(&dest1, &src1, 4)
        self.assertEqual(dest1, src1)

        # 测试浮点复制
        rn = random.random() + rand_int()
        cdef double dest2, src2 = rn
        qstring.q_memcpy(&dest2, &src2, sizeof(double))
        self.assertEqual(dest2, src2)

        # 测试垃圾内存复制
        mem_size = 10
        cdef double dest3, src3
        qstring.q_memcpy(&dest3, &src3, sizeof(double))
        self.assertEqual(dest3, src3)

    def test_memset(self):
        cdef char r[10]
        qstring.q_memset(r, 0, 10)
        self.assertEqual(r, '')

        cdef int t;
        qstring.q_memset(&t, 0, sizeof(int))
        self.assertEqual(t, 0)

    def test_bzero(self):
        cdef char r[10]
        qstring.q_bzero(r, 10)
        self.assertEqual(r, '')

    def test_utoa(self):
        cdef char *string = <char*> malloc(33)
        rand_int = random.randint(0, self.max_uint64)
        qstring.q_utoa(rand_int, string)
        self.assertEqual(string, str(rand_int))
        free(string)

    def test_itoa_limit(self):
        cdef char *string = <char*> malloc(33)
        it = 0
        qstring.q_utoa(it, string)
        self.assertEqual(string, str(it))
        free(string)

    def test_hex(self):
        rand_int = random.randint(0, self.max_uint64)
        cdef char *string = <char*> malloc(65)
        qstring.hex(rand_int, string)
        self.assertEqual(string, hex(rand_int)[2:])

    def test_hex_limit(self):
        cdef char *string = <char*> malloc(33)
        it = 0
        qstring.hex(it, string)
        self.assertEqual(string, hex(it)[2:])

    def test_memset16(self):
        rn = random.randint(0, self.max_uint16)
        _len = 16
        cdef unsigned short *p = <unsigned short *> malloc(_len * 2)
        cdef unsigned short *p2 = <unsigned short *> malloc(_len * 2)
        for i in range(_len):
            p[i] = rn

        qstring.q_memset16(p2, rn, _len)
        for i in range(_len):
            self.assertEqual(p[i], p2[i])

        free(p)
        free(p2)

    def test_reverse(self):
        string = tool.random_str(20)
        cdef char *string2 = <char *> malloc(len(string) + 1)
        strcpy(string2, string)
        qstring.reverse(string2, len(string) - 1)
        self.assertEqual(string2, string[::-1])


Test().run()
