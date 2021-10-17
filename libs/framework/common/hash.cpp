#include "hash.h"

#include <stdint.h>

// from nuklear ui

#define NK_INT8 int8_t
#define NK_UINT8 uint8_t
#define NK_INT16 int16_t
#define NK_UINT16 uint16_t
#define NK_INT32 int32_t
#define NK_UINT32 uint32_t
#define NK_SIZE_TYPE uintptr_t
#define NK_POINTER_TYPE uintptr_t

typedef NK_INT8 nk_char;
typedef NK_UINT8 nk_uchar;
typedef NK_UINT8 nk_byte;
typedef NK_INT16 nk_short;
typedef NK_UINT16 nk_ushort;
typedef NK_INT32 nk_int;
typedef NK_UINT32 nk_uint;
typedef NK_SIZE_TYPE nk_size;
typedef NK_POINTER_TYPE nk_ptr;

typedef nk_uint nk_hash;
typedef nk_uint nk_flags;
typedef nk_uint nk_rune;

nk_hash nk_murmur_hash(const void* key, int len, nk_hash seed)
{
    /* 32-Bit MurmurHash3: https://code.google.com/p/smhasher/wiki/MurmurHash3*/
#define NK_ROTL(x, r) ((x) << (r) | ((x) >> (32 - r)))

    nk_uint h1 = seed;
    nk_uint k1;
    const nk_byte* data = (const nk_byte*)key;
    const nk_byte* keyptr = data;
    nk_byte* k1ptr;
    const int bsize = sizeof(k1);
    const int nblocks = len / 4;

    const nk_uint c1 = 0xcc9e2d51;
    const nk_uint c2 = 0x1b873593;
    const nk_byte* tail;
    int i;

    /* body */
    if (!key)
        return 0;
    for (i = 0; i < nblocks; ++i, keyptr += bsize) {
        k1ptr = (nk_byte*)&k1;
        k1ptr[0] = keyptr[0];
        k1ptr[1] = keyptr[1];
        k1ptr[2] = keyptr[2];
        k1ptr[3] = keyptr[3];

        k1 *= c1;
        k1 = NK_ROTL(k1, 15);
        k1 *= c2;

        h1 ^= k1;
        h1 = NK_ROTL(h1, 13);
        h1 = h1 * 5 + 0xe6546b64;
    }

    /* tail */
    tail = (const nk_byte*)(data + nblocks * 4);
    k1 = 0;
    switch (len & 3) {
    case 3:
        k1 ^= (nk_uint)(tail[2] << 16); /* fallthrough */
    case 2:
        k1 ^= (nk_uint)(tail[1] << 8u); /* fallthrough */
    case 1:
        k1 ^= tail[0];
        k1 *= c1;
        k1 = NK_ROTL(k1, 15);
        k1 *= c2;
        h1 ^= k1;
        break;
    default:
        break;
    }

    /* finalization */
    h1 ^= (nk_uint)len;
    /* fmix32 */
    h1 ^= h1 >> 16;
    h1 *= 0x85ebca6b;
    h1 ^= h1 >> 13;
    h1 *= 0xc2b2ae35;
    h1 ^= h1 >> 16;

#undef NK_ROTL
    return h1;
}

unsigned int murmur_hash(const void* key, int len, unsigned int seed)
{
    return nk_murmur_hash(key, len, seed);
}