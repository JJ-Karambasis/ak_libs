#ifdef _MSC_VER
#include <intrin.h>
#endif

#ifndef AK_STL_Assert
#include <assert.h>
#define AK_STL_Assert assert
#endif

#if !defined(AK_STL_Memset) || !defined(AK_STL_Memcpy)
#include <string.h>

#if !defined(AK_STL_Memset)
#define AK_STL_Memset memset
#endif

#if !defined(AK_STL_Memcpy)
#define AK_STL_Memcpy memcpy
#endif

#endif

#if !defined(AK_STL_Malloc) || !defined(AK_STL_Realloc) || !defined(AK_STL_Free)
#include <stdlib.h>

//NOTE(EVERYONE): If any of the allocations functions are not defined we must use the default ones otherwise things will UB
#undef AK_STL_Malloc
#undef AK_STL_Realloc
#undef AK_STL_Free

#define AK_STL_Malloc  malloc
#define AK_STL_Free    free
#define AK_STL_Realloc realloc

#endif

#include "allocators.cpp"