#pragma once

#include "MantidNexus/DllConfig.h"
#include "MantidNexus/NexusFile_fwd.h"
#include "MantidNexus/napi.h"

#include <hdf5.h>

MANTID_NEXUS_DLL std::string getObjectAddress(hid_t obj);

MANTID_NEXUS_DLL std::string buildCurrentAddress(NexusFile5 &fid);

MANTID_NEXUS_DLL NXnumtype hdf5ToNXType(H5T_class_t tclass, hid_t atype);

MANTID_NEXUS_DLL hid_t nxToHDF5Type(NXnumtype datatype);

MANTID_NEXUS_DLL hid_t h5MemType(hid_t atype);

/** Determine if a file can be opened using HDF5
 * \param filename full path to the file to be checked
 * \return true if it exists and can be opened withHDF5, otherwise false
 */
MANTID_NEXUS_DLL bool canBeOpened(std::string const &filename);
