#include "MantidNexus/UniqueID.h"
#include "MantidNexus/DllConfig.h"
#include <hdf5.h>

namespace Mantid::Nexus {

template class MANTID_NEXUS_DLL UniqueID<&H5Gclose>;
template class MANTID_NEXUS_DLL UniqueID<&H5Dclose>;
template class MANTID_NEXUS_DLL UniqueID<&H5Tclose>;
template class MANTID_NEXUS_DLL UniqueID<&H5Sclose>;
template class MANTID_NEXUS_DLL UniqueID<&H5Aclose>;
template class MANTID_NEXUS_DLL UniqueID<&H5Pclose>;

} // namespace Mantid::Nexus
