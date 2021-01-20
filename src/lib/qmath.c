#include "stdint.h"
#include "qlib.h"
#include "qstring.h"
#include "limits.h"

float _q_ceilf(float _arg) {
    uint32_t be_mask = generate_mask(8);
    uint32_t frac_mask = generate_mask(23);

    uint32_t arg;
    q_memcpy(&arg, &_arg, sizeof(uint32_t));
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
    q_memcpy(&_arg, &arg, sizeof(uint32_t));
    return sign_bit == 0 ? _arg + 1 : _arg;
}

float q_ceilf(float _arg) {
    if (_arg < (float) (LONG_MAX)) {
        int64_t t = (int64_t) _arg;
        if (_arg < 0) return t;
        return _arg == t ? _arg : (float) (t + 1);
    }
    return _q_ceilf(_arg);
}

// unsigned 32 位除法,取 ceil, 第一个参数为被除数
uint32_t divUc(uint32_t dividend, uint32_t divider) {
    return (uint32_t) q_ceilf((float) dividend / divider);
}