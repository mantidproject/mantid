#include <vector>
#include <sstream>
#include <napi.h>
#include <stdlib.h>
#ifdef _WIN32
#include <io.h>
#ifndef F_OK
#define F_OK 0	/* MinGW has this defined, Visual Studio doesn't */
#endif
#endif /* _WIN32 */
#include "MantidNexus/NeXusUtils.h"

// quick and dirty illustation of NeXus writing of a workspace
// I've got better code I will bring in from elsewhere later

void testNX()
{
    NXhandle h;
    NXopen("dummy.nxs", NXACC_READ, &h);
}

static void write_data(NXhandle h, const char* name, const std::vector<double>& v)
{
    int dims_array[1] = { v.size() };
    NXmakedata(h, name, NX_FLOAT64, 1, dims_array);
    NXopendata(h, "x");
    NXputdata(h, (void*)&(v[0]));
    NXclosedata(h);
}

void writeEntry1D(const std::string& filename, const std::string& entryname, const std::vector<double>& x, const std::vector<double>& y, const std::vector<double>& e)
{
    NXhandle h;
    NXaccess mode;
    if (access(filename.c_str(), F_OK) == 0)
    {
        mode = NXACC_RDWR;
    }
    else
    {
        mode = NXACC_CREATE5;
    }
    NXopen(filename.c_str(), mode, &h);
    NXmakegroup(h, "entry1", "NXentry");
    NXopengroup(h, "entry1", "NXentry");
    NXmakegroup(h, entryname.c_str(), "NXdata");
    NXopengroup(h, entryname.c_str(), "NXdata");
    write_data(h, "x", x);
    write_data(h, "y", y);
    write_data(h, "e", e);
    NXclosegroup(h);
    NXclosegroup(h);
    NXclose(&h);
}
