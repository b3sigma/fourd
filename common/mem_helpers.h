#ifndef _FD_MEM_HELPERS_H_
#define _FD_MEM_HELPERS_H_

#include <stdlib.h>
#include <malloc.h>

#define FD_MEM_ALIGN_AMOUNT (16)

#if defined(_MSC_VER) // windows
  #define ALIGNED_ALLOC_NEW_DEL_OVERRIDE \
      void* operator new(size_t size) { return _aligned_malloc(size, FD_MEM_ALIGN_AMOUNT); } \
      void operator delete(void* p) { _aligned_free(p); }
#else // linux
#define ALIGNED_ALLOC_NEW_DEL_OVERRIDE \
    void* operator new(size_t size) { void* ptr; \
      return (0 == posix_memalign(&ptr, FD_MEM_ALIGN_AMOUNT, size)) ? \
          ptr : NULL; } \
    void operator delete(void* p) { free(p); }
#endif //platform

#endif //_FD_MEM_HELPERS_H_
