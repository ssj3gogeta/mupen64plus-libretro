#ifndef _SHUFFLE_H
#define _SHUFFLE_H

/*
 * for ANSI compliance (null INLINE attribute if not already set to `inline`)
 * Include "rsp.h" for active, non-ANSI inline definition.
 */
#ifndef INLINE
#define INLINE
#endif

#ifndef ARCH_MIN_SSE2
/*
 * vector-scalar element decoding
 * Obsolete.  Consider using at least the SSE2 algorithms instead.
 */
static const int ei[16][8] = {
    { 00, 01, 02, 03, 04, 05, 06, 07 }, /* none (vector-only operand) */
    { 00, 01, 02, 03, 04, 05, 06, 07 },
    { 00, 00, 02, 02, 04, 04, 06, 06 }, /* 0Q */
    { 01, 01, 03, 03, 05, 05, 07, 07 }, /* 1Q */
    { 00, 00, 00, 00, 04, 04, 04, 04 }, /* 0H */
    { 01, 01, 01, 01, 05, 05, 05, 05 }, /* 1H */
    { 02, 02, 02, 02, 06, 06, 06, 06 }, /* 2H */
    { 03, 03, 03, 03, 07, 07, 07, 07 }, /* 3H */
    { 00, 00, 00, 00, 00, 00, 00, 00 }, /* 0 */
    { 01, 01, 01, 01, 01, 01, 01, 01 }, /* 1 */
    { 02, 02, 02, 02, 02, 02, 02, 02 }, /* 2 */
    { 03, 03, 03, 03, 03, 03, 03, 03 }, /* 3 */
    { 04, 04, 04, 04, 04, 04, 04, 04 }, /* 4 */
    { 05, 05, 05, 05, 05, 05, 05, 05 }, /* 5 */
    { 06, 06, 06, 06, 06, 06, 06, 06 }, /* 6 */
    { 07, 07, 07, 07, 07, 07, 07, 07 }  /* 7 */
};

int sub_mask[16] = {
    0x0,
    0x0,
    0x1, 0x1,
    0x3, 0x3, 0x3, 0x3,
    0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7
};

INLINE static void SHUFFLE_VECTOR(short* VD, short* VT, const int e)
{
    short SV[8];
    register int i, j;
#if (0 == 0)
    j = sub_mask[e];
    for (i = 0; i < N; i++)
        SV[i] = VT[(i & ~j) | (e & j)];
#else
    if (e & 0x8)
        for (i = 0; i < N; i++)
            SV[i] = VT[(i & 0x0) | (e & 0x7)];
    else if (e & 0x4)
        for (i = 0; i < N; i++)
            SV[i] = VT[(i & 0xC) | (e & 0x3)];
    else if (e & 0x2)
        for (i = 0; i < N; i++)
            SV[i] = VT[(i & 0xE) | (e & 0x1)];
    else /* if ((e == 0b0000) || (e == 0b0001)) */
        for (i = 0; i < N; i++)
            SV[i] = VT[(i & 0x7) | (e & 0x0)];
#endif
    for (i = 0; i < N; i++)
        *(VD + i) = *(SV + i);
    return;
}
#else
#ifdef ARCH_MIN_SSSE3
static const unsigned char smask[16][16] = {
    {0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xA,0xB,0xC,0xD,0xE,0xF},
    {0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xA,0xB,0xC,0xD,0xE,0xF},
    {0x0,0x1,0x0,0x1,0x4,0x5,0x4,0x5,0x8,0x9,0x8,0x9,0xC,0xD,0xC,0xD},
    {0x2,0x3,0x2,0x3,0x6,0x7,0x6,0x7,0xA,0xB,0xA,0xB,0xE,0xF,0xE,0xF},
    {0x0,0x1,0x0,0x1,0x0,0x1,0x0,0x1,0x8,0x9,0x8,0x9,0x8,0x9,0x8,0x9},
    {0x2,0x3,0x2,0x3,0x2,0x3,0x2,0x3,0xA,0xB,0xA,0xB,0xA,0xB,0xA,0xB},
    {0x4,0x5,0x4,0x5,0x4,0x5,0x4,0x5,0xC,0xD,0xC,0xD,0xC,0xD,0xC,0xD},
    {0x6,0x7,0x6,0x7,0x6,0x7,0x6,0x7,0xE,0xF,0xE,0xF,0xE,0xF,0xE,0xF},
    {0x0,0x1,0x0,0x1,0x0,0x1,0x0,0x1,0x0,0x1,0x0,0x1,0x0,0x1,0x0,0x1},
    {0x2,0x3,0x2,0x3,0x2,0x3,0x2,0x3,0x2,0x3,0x2,0x3,0x2,0x3,0x2,0x3},
    {0x4,0x5,0x4,0x5,0x4,0x5,0x4,0x5,0x4,0x5,0x4,0x5,0x4,0x5,0x4,0x5},
    {0x6,0x7,0x6,0x7,0x6,0x7,0x6,0x7,0x6,0x7,0x6,0x7,0x6,0x7,0x6,0x7},
    {0x8,0x9,0x8,0x9,0x8,0x9,0x8,0x9,0x8,0x9,0x8,0x9,0x8,0x9,0x8,0x9},
    {0xA,0xB,0xA,0xB,0xA,0xB,0xA,0xB,0xA,0xB,0xA,0xB,0xA,0xB,0xA,0xB},
    {0xC,0xD,0xC,0xD,0xC,0xD,0xC,0xD,0xC,0xD,0xC,0xD,0xC,0xD,0xC,0xD},
    {0xE,0xF,0xE,0xF,0xE,0xF,0xE,0xF,0xE,0xF,0xE,0xF,0xE,0xF,0xE,0xF}
};

INLINE static void SHUFFLE_VECTOR(short* VD, short* VT, const int e)
{ /* SSSE3 shuffling method was written entirely by CEN64 author MarathonMan. */
    __m128i xmm;
    __m128i key;

    xmm = _mm_load_si128((__m128i *)VT);
    key = _mm_load_si128((__m128i *)smask[e & 0xF]);
    xmm = _mm_shuffle_epi8(xmm, key);
    _mm_store_si128((__m128i *)VD, xmm);
    return;
}
#else
#define B(x)    ((x) & 3)
#define SHUFFLE(a,b,c,d)    ((B(d)<<6) | (B(c)<<4) | (B(b)<<2) | (B(a)<<0))

static const int simm[16] = {
    SHUFFLE(00, 01, 02, 03), /* vector operands */
    SHUFFLE(00, 01, 02, 03),
    SHUFFLE(00, 00, 02, 02), /* scalar quarters */
    SHUFFLE(01, 01, 03, 03),
    SHUFFLE(00, 00, 00, 00), /* scalar halves */
    SHUFFLE(01, 01, 01, 01),
    SHUFFLE(02, 02, 02, 02),
    SHUFFLE(03, 03, 03, 03),
    SHUFFLE(00, 00, 00, 00), /* scalar wholes */
    SHUFFLE(01, 01, 01, 01),
    SHUFFLE(02, 02, 02, 02),
    SHUFFLE(03, 03, 03, 03),
    SHUFFLE(04, 04, 04, 04),
    SHUFFLE(05, 05, 05, 05),
    SHUFFLE(06, 06, 06, 06),
    SHUFFLE(07, 07, 07, 07)
};

static __m128i shuffle_none(__m128i xmm)
{/*
    const int order = simm[0x0];

    xmm = _mm_shufflehi_epi16(xmm, order);
    xmm = _mm_shufflelo_epi16(xmm, order);*/
    return (xmm);
}
static __m128i shuffle_0q(__m128i xmm)
{
    const int order = simm[0x2];

    xmm = _mm_shufflehi_epi16(xmm, order);
    xmm = _mm_shufflelo_epi16(xmm, order);
    return (xmm);
}
static __m128i shuffle_1q(__m128i xmm)
{
    const int order = simm[0x3];

    xmm = _mm_shufflehi_epi16(xmm, order);
    xmm = _mm_shufflelo_epi16(xmm, order);
    return (xmm);
}
static __m128i shuffle_0h(__m128i xmm)
{
    const int order = simm[0x4];

    xmm = _mm_shufflehi_epi16(xmm, order);
    xmm = _mm_shufflelo_epi16(xmm, order);
    return (xmm);
}
static __m128i shuffle_1h(__m128i xmm)
{
    const int order = simm[0x5];

    xmm = _mm_shufflehi_epi16(xmm, order);
    xmm = _mm_shufflelo_epi16(xmm, order);
    return (xmm);
}
static __m128i shuffle_2h(__m128i xmm)
{
    const int order = simm[0x6];

    xmm = _mm_shufflehi_epi16(xmm, order);
    xmm = _mm_shufflelo_epi16(xmm, order);
    return (xmm);
}
static __m128i shuffle_3h(__m128i xmm)
{
    const int order = simm[0x7];

    xmm = _mm_shufflehi_epi16(xmm, order);
    xmm = _mm_shufflelo_epi16(xmm, order);
    return (xmm);
}
static __m128i shuffle_0w(__m128i xmm)
{
    const int order = simm[0x8];

    xmm = _mm_shufflelo_epi16(xmm, order);
    xmm = _mm_unpacklo_epi16(xmm, xmm);
    return (xmm);
}
static __m128i shuffle_1w(__m128i xmm)
{
    const int order = simm[0x9];

    xmm = _mm_shufflelo_epi16(xmm, order);
    xmm = _mm_unpacklo_epi16(xmm, xmm);
    return (xmm);
}
static __m128i shuffle_2w(__m128i xmm)
{
    const int order = simm[0xA];

    xmm = _mm_shufflelo_epi16(xmm, order);
    xmm = _mm_unpacklo_epi16(xmm, xmm);
    return (xmm);
}
static __m128i shuffle_3w(__m128i xmm)
{
    const int order = simm[0xB];

    xmm = _mm_shufflelo_epi16(xmm, order);
    xmm = _mm_unpacklo_epi16(xmm, xmm);
    return (xmm);
}
static __m128i shuffle_4w(__m128i xmm)
{
    const int order = simm[0xC];

    xmm = _mm_shufflehi_epi16(xmm, order);
    xmm = _mm_unpackhi_epi16(xmm, xmm);
    return (xmm);
}
static __m128i shuffle_5w(__m128i xmm)
{
    const int order = simm[0xD];

    xmm = _mm_shufflehi_epi16(xmm, order);
    xmm = _mm_unpackhi_epi16(xmm, xmm);
    return (xmm);
}
static __m128i shuffle_6w(__m128i xmm)
{
    const int order = simm[0xE];

    xmm = _mm_shufflehi_epi16(xmm, order);
    xmm = _mm_unpackhi_epi16(xmm, xmm);
    return (xmm);
}
static __m128i shuffle_7w(__m128i xmm)
{
    const int order = simm[0xF];

    xmm = _mm_shufflehi_epi16(xmm, order);
    xmm = _mm_unpackhi_epi16(xmm, xmm);
    return (xmm);
}

static __m128i (*SSE2_SHUFFLE_16[16])(__m128i) = {
    shuffle_none, shuffle_none,
    shuffle_0q, shuffle_1q,
    shuffle_0h, shuffle_1h, shuffle_2h, shuffle_3h,
    shuffle_0w, shuffle_1w, shuffle_2w, shuffle_3w,
    shuffle_4w, shuffle_5w, shuffle_6w, shuffle_7w
};

INLINE static void SHUFFLE_VECTOR(short* VD, short* VT, const int e)
{
    __m128i xmm;

    xmm = _mm_load_si128((__m128i *)VT);
    xmm = SSE2_SHUFFLE_16[e](xmm);
    _mm_store_si128((__m128i *)VD, xmm);
    return;
}
#endif
#endif
#endif
