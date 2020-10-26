#ifndef UTIL_H
#define UTIL_H

#define PURE __attribute__((pure))
#define CONST __attribute__((const))
#define NODISCARD __attribute__((warn_unused_result))
#define ALWAYS_INLINE __attribute__((always_inline))
#define NOINLINE __attribute__((noinline))
#define UNUSED(x) x##_UNUSED __attribute__((unused))
#define NONNULL(x) __attribute__((nonnull x))
#define HLTLOOP asm volatile("1: hlt; jmp 1b")
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))

#endif /* UTIL_H */
