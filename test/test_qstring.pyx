# cython: c_string_type=unicode, c_string_encoding=utf8
# 设置自动编码, 不要删除上面的注释

from libc.stdlib cimport malloc, free
from libc.string cimport strcpy
import random
cimport qstring
import tool


class Test(tool.TestCase):
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


Test().run()
