#include <napi.h>
#include "NeXusUtils.h"

void testNX()
{
    NXhandle h;
    NXopen("dummy.nxs", NXACC_READ, &h);
}
