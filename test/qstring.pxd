# cython 构建目录被设置在 ./build,而不是 ./， 因此不要修改这里的导入路径

from libc.stdint cimport uint64_t, uint16_t, uint32_t
cdef extern from "../../src/include/qstring.h":
    ctypedef bint bool
    size_t q_strlen(const char *str)

    char *q_strcat(char *dest, const char *src)

    char *q_strncat(char *dest, const char *src, size_t n)

    bool q_strcmp(const char *s1, const char *s2)

    void *q_memcpy(void *dest, const void *src, size_t n)

    void *q_memset(void *s, int c, size_t n)

    void q_bzero(void *s, size_t n)
    void q_utoa(uint64_t value, char *str)
    void hex(uint64_t n, char *str)

    void reverse(char *s, uint32_t n)
    void *q_memset16(void *s, uint16_t c, size_t n)
