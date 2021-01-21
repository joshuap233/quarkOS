# cython: c_string_type=unicode, c_string_encoding=utf8
# 设置自动编码, 不要删除上面的注释

from libc.stdlib cimport malloc, free
from libc.string cimport strcpy, strcmp
from libc.stdint cimport uintptr_t
import random
cimport qstring
import tool


class Test(tool.TestCase):
    max_int = 2 ** 31 - 1
    min_int = -2 ** 31

    def test_strlen(self):
        string = tool.random_str()
        self.assertEqual(qstring.q_strlen(string), len(string))

    def test_strcat(self):
        cdef char*s
        tail = "tail"
        string = tool.random_str()
        s = <char *> malloc(len(string) + 1 + len(tail))
        strcpy(s, string)
        qstring.q_strcat(s, tail)
        self.assertEqual(s, string + tail)
        free(s)

    def test_strncat(self):
        cdef char*s
        tail = "tail"
        string = tool.random_str()
        rani = random.randint(0, len(tail))
        s = <char *> malloc(len(string) + 1 + len(tail))
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


Test().run()
