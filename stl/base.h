#ifndef BASE_H
#define BASE_H

namespace ak_stl
{
    template <typename type> inline type Min(type A, type B) { return A < B ? A : B; }
    template <typename type> inline type Max(type A, type B) { return A > B ? A : B; }
    template <typename type> inline bool Is_Pow2(type x) { return ((x != 0) && ((x & (x - 1)) == 0)); }
    template <typename type> inline void Array_Copy(type* Dst, const type* Src, size_t Count)
    {
        for(size_t i = 0; i < Count; i++) Dst[i] = Src[i];
    }
}

#endif
