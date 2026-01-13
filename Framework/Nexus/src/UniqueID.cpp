#include "MantidNexus/UniqueID.h"
#include "MantidNexus/DllConfig.h"
#include <hdf5.h>

namespace Mantid::Nexus {

// ******************************************************************
// EXPORTS
// ******************************************************************

template class MANTID_NEXUS_DLL UniqueID<&H5Fclose>;
template class MANTID_NEXUS_DLL UniqueID<&H5Gclose>;
template class MANTID_NEXUS_DLL UniqueID<&H5Dclose>;
template class MANTID_NEXUS_DLL UniqueID<&H5Tclose>;
template class MANTID_NEXUS_DLL UniqueID<&H5Sclose>;
template class MANTID_NEXUS_DLL UniqueID<&H5Aclose>;
template class MANTID_NEXUS_DLL UniqueID<&H5Pclose>;
// this will be used in Nexus::File
template class MANTID_NEXUS_DLL SharedID<&H5Fclose>;

} // namespace Mantid::Nexus
