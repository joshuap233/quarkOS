# cython 构建目录被设置在 ./build,而不是 ./， 因此不要修改这里的导入路径
from libc.stdint cimport uint64_t, uint8_t, uint32_t

cdef extern from "../../src/include/qlib.h":
    void q_utoa(uint64_t value, char *str)
    void hex(uint64_t n, char *str)
