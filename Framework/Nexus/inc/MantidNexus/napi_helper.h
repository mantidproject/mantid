#pragma once

#include "MantidNexus/DllConfig.h"
#include "MantidNexus/NexusFile_fwd.h"
#include "MantidNexus/napi.h"

#include <hdf5.h>

MANTID_NEXUS_DLL std::string getObjectAddress(hid_t const obj);

MANTID_NEXUS_DLL std::string buildCurrentAddress(NexusFile5 const &fid);

MANTID_NEXUS_DLL NXnumtype hdf5ToNXType(H5T_class_t tclass, hid_t atype);

MANTID_NEXUS_DLL hid_t nxToHDF5Type(NXnumtype datatype);

MANTID_NEXUS_DLL hid_t h5MemType(hid_t atype);
