# cython 构建目录被设置在 ./build,而不是 ./， 因此不要修改这里的导入路径
from libc.stdint cimport uint32_t, uint8_t

cdef extern from "../../src/include/qlib.h":
    void q_itoa(uint32_t value, char *str)
    void hex(uint32_t n, char *str)
    # uint32_t generate_mask(uint8_t num)
