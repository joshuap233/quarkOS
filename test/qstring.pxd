# cython 构建目录被设置在 ./build,而不是 ./， 因此不要修改这里的导入路径
cdef extern from "../../src/include/qstring.h":
    ctypedef bint bool
    size_t q_strlen(const char *str)

    char *q_strcat(char *dest, const char *src)

    char *q_strncat(char *dest, const char *src, size_t n)

    bool q_strcmp(const char *s1, const char *s2)

    void *q_memcpy(void *dest, const void *src, size_t n)
