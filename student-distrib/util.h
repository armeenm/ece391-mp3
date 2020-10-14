#ifndef UTIL_H
#define UTIL_H

#define PURE __attribute__((pure))
#define CONST __attribute__((const))
#define NODISCARD __attribute__((warn_unused_result))
#define UNUSED(x) x##_UNUSED __attribute__((unused))
#define NONNULL(x) __attribute__((nonnull x))

#endif /* UTIL_H */
