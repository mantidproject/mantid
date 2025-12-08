#include "MantidNexus/UniqueID.h"
#include "MantidNexus/DllConfig.h"
#include <hdf5.h>

namespace Mantid::Nexus {

template class MANTID_NEXUS_DLL UniqueID<&H5Fclose>;
template class MANTID_NEXUS_DLL UniqueID<&H5Gclose>;
template class MANTID_NEXUS_DLL UniqueID<&H5Dclose>;
template class MANTID_NEXUS_DLL UniqueID<&H5Tclose>;
template class MANTID_NEXUS_DLL UniqueID<&H5Sclose>;

} // namespace Mantid::Nexus
