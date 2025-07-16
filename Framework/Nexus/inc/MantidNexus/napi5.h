#pragma once

/* Hide deprecated API from HDF5 versions before 1.8
 * Required to build on Ubuntu 12.04 */

#include "MantidNexus/NexusFile_fwd.h"
#include <hdf5.h>

/* HDF5 interface */

NXstatus NX5getdata(NXhandle handle, void *data);
NXstatus NX5getinfo64(NXhandle handle, std::size_t &rank, Mantid::Nexus::DimVector &dims, NXnumtype &datatype);

herr_t group_info(hid_t loc_id, const char *name, const H5L_info_t *unused, void *opdata);
