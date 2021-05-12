#include "stdio.h"
#include "stdint.h"

typedef uint64_t u64_t;
typedef uint32_t u32_t;
typedef uint16_t u16_t;
typedef uint8_t u8_t;

typedef struct {
     int counter;
} atomic_t;

struct foo {
    u8_t flags;
};

static void atomic_set(atomic_t *v, int i) {
    v->counter = i;
}

int main() {
//    atomic_t v;
//    atomic_set(&v, 1);
    printf("%ld", sizeof(struct foo));
}