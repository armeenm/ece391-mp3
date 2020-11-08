#ifndef UTIL_H
#define UTIL_H

#define NORETURN __attribute__((noreturn))
#define PURE __attribute__((pure))
#define CONST __attribute__((const))
#define NODISCARD __attribute__((warn_unused_result))
#define ALWAYS_INLINE __attribute__((always_inline))
#define NOINLINE __attribute__((noinline))
#define PACKED __attribute__((packed))
#define UNUSED(x) x##_UNUSED __attribute__((unused))
#define NONNULL(x) __attribute__((nonnull x))
#define HLTLOOP asm volatile("1: hlt; jmp 1b")
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))

static inline void ALWAYS_INLINE NORETURN crash(void) {
  asm volatile("int $15");

  /* We'll never get back here
   * This is just to avoid compiler warnings
   */
  for (;;)
    ;
}

#define NIMPL                                                                                      \
  do {                                                                                             \
    printf("Unimplemented function %s@%s:%d called!\n", __FUNCTION__, __FILE__, __LINE__);         \
    crash();                                                                                       \
  } while (0)

#endif /* UTIL_H */
