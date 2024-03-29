#include <types.h>
#include <lib/qlib.h>
#include <lib/qstring.h>
#include <limits.h>
#include <lib/qmath.h>

float _q_ceilf(float _arg) {
    uint32_t be_mask = BIT_MASK(uint32_t ,8);
    uint32_t frac_mask = BIT_MASK(uint32_t ,23);

    uint32_t arg;
    memcpy(&arg, &_arg, sizeof(uint32_t));
    // 浮点符号位
    uint32_t sign_bit = arg >> 31;
    // 浮点指数位
    int32_t be_bit = ((arg >> 23) & be_mask) - 127;
    // 浮点尾数位
    uint32_t frac_bit = arg & frac_mask;

    if (be_bit < 0) {
        if (frac_bit == 0 || sign_bit == 1) return 0;
        return 1;
    }
    frac_bit = (frac_bit << be_bit) & frac_mask;
    if (frac_bit == 0) return _arg;
    // 清除 arg 真正表示浮点的位
    arg ^= (frac_bit >> be_bit);
    memcpy(&_arg, &arg, sizeof(uint32_t));
    return sign_bit == 0 ? _arg + 1 : _arg;
}

float ceilf(float _arg) {
    if (_arg < (float) (LONG_MAX)) {
        int64_t t = (int64_t) _arg;
        if (_arg < 0) return t;
        return _arg == t ? _arg : (float) (t + 1);
    }
    return _ceilf(_arg);
}
