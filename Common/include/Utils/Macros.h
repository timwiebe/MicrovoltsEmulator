#ifndef MACROS_UTILS_CUSTOM_H
#define MACROS_UTILS_CUSTOM_H


#if defined(_MSC_VER)
#define PACK_PUSH(n) __pragma(pack(push, n))
#define PACK_POP()   __pragma(pack(pop))
#elif defined(__GNUC__) || defined(__clang__)
#define STRINGIFY(x) #x
#define PACK_PUSH(n) _Pragma(STRINGIFY(pack(push, n)))
#define PACK_POP()   _Pragma("pack(pop)")
#else
#define PACK_PUSH(n)
#define PACK_POP()
#endif


#endif