#include "vu.h"

INLINE static void do_eq(short* VD, short* VS, short* VT)
{
    register int i;

    for (i = 0; i < N; i++)
        clip[i] = 0;
    for (i = 0; i < N; i++)
        comp[i] = (VS[i] == VT[i]);
    for (i = 0; i < N; i++)
        comp[i] = comp[i] & (ne[i] ^ 1);
#if (0)
    merge(VACC_L, comp, VS, VT); /* correct but redundant */
#else
    vector_copy(VACC_L, VT);
#endif
    vector_copy(VD, VACC_L);

    for (i = 0; i < N; i++)
        ne[i] = 0;
    for (i = 0; i < N; i++)
        co[i] = 0;
    return;
}

static void VEQ(int vd, int vs, int vt, int e)
{
    short ST[N];

    SHUFFLE_VECTOR(ST, VR[vt], e);
    do_eq(VR[vd], VR[vs], ST);
    return;
}
