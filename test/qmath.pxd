# cython 构建目录被设置在 ./build,而不是 ./， 因此不要修改这里的导入路径
from libc.stdint cimport uint32_t

cdef extern from "../../src/include/qmath.h":
    double q_ceilf(double arg)
    double _q_ceilf(double arg)
    double q_floorf(double arg)
    uint32_t divUc(uint32_t dividend, uint32_t divider)
    float q_ceilf(float _arg)

