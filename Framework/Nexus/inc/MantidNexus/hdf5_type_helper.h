#pragma once

#include "MantidNexus/DllConfig.h"
#include "MantidNexus/NexusFile_fwd.h"

#include <hdf5.h>

MANTID_NEXUS_DLL NXnumtype hdf5ToNXType(H5T_class_t tclass, hid_t atype);

MANTID_NEXUS_DLL hid_t nxToHDF5Type(NXnumtype datatype);

MANTID_NEXUS_DLL hid_t h5MemType(hid_t atype);
